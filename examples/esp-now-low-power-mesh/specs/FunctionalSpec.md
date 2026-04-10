# ESP-NOW Low-Power Mesh with WiFi RSSI Sensing

## Overview

Infrastructure-free multi-hop sensor network. Four roles run on the same firmware,
selected by a Kconfig option: **sensor node** (sleeps 60 seconds between transmissions,
wakes to scan nearby WiFi access points for signal strength, packages real RSSI data
into an ESP-NOW packet, routes the packet toward the gateway through one or more hops,
then sleeps again); **relay node** (always-on, receives sensor packets and forwards them
toward the gateway, adding a hop to the path, deduplicating retransmits); **gateway
node** (always-on, broadcasts a beacon so nodes can discover it without a hardcoded MAC
address, receives and logs all packets, records hop count and per-SSID RSSI values); and
**sensor-relay node** (uses ESP-IDF light sleep to forward other sensors' packets while
dormant, then periodically wakes on a timer to run its own WiFi scan and transmit its own
payload — bridging the gap between deep-sleeping sensors and always-on relays in
power-constrained deployments).

This example demonstrates:

- True multi-hop ESP-NOW mesh routing (not hub-and-spoke)
- Dynamic gateway discovery via beacon — no compile-time MAC hardcoding
- Real sensor data: WiFi RSSI from up to three configured SSIDs
- Deep sleep duty cycling with RTC memory used for both counters and topology state
- Per-packet hop counting as evidence of relay path usage

## Supported Boards

> Default board: Seeed XIAO ESP32S3 (`seeed/xiao-esp32s3.md`, target `esp32s3`)

The Supported Boards table and Per-Board Behavior table in the generated README are
**auto-discovered** by scanning `board-specs/` at documentation-generation time and
applying the Board Compatibility Checklist below. Do not maintain a hardcoded board
table here — add new boards via `esp32-board-spec-generator` and re-run
`esp32-sdd-documentation-generator`.

## Board Compatibility Checklist

Use this checklist to evaluate any board in `board-specs/`. The documentation generator
applies these checks automatically; they are also useful for manual review when a new
board is added. A board is compatible with this example if and only if it passes all
required checks. All WiFi-capable boards in this catalog also support the SENSOR_RELAY
role — light sleep + ESP-NOW receive is available on every WiFi-capable ESP32 SoC
supported by ESP-IDF 5.x.

### Required checks (any failure = incompatible)

- [ ] **WiFi present**: the board-spec documents a WiFi radio (802.11b/g/n or ax).
  ESP-NOW is a WiFi-layer protocol — boards without WiFi (e.g. ESP32-H2) cannot
  participate. Confirm in the board-spec's Module / SoC table.
- [ ] **Deep sleep with RTC timer**: the board-spec documents a deep sleep current
  figure and does not flag timer wakeup as unsupported. All current ESP32-family SoCs
  support RTC timer wakeup; verify for any new SoC family.
- [ ] **idf.py target listed as Supported** in `shared-specs/esp-idf-version.md`.
  If the target row is absent or marked unsupported, do not use this board until the
  shared spec is updated.
- [ ] **At least one user-controllable LED**: the board-spec documents a GPIO connected
  to an LED (simple or addressable). The example uses the LED for send-success, send-
  failure, and packet-received indication; it cannot run meaningfully without one.

### Adaptation checks (compatible, but requires per-board sdkconfig or code change)

- [ ] **USB console type**: check the "USB Serial / Console" section of the board-spec.
  - USB-UART bridge (e.g. CP2104): no sdkconfig entry needed.
  - Native USB Serial/JTAG (ESP32-C3/C6/C5/H2): add `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y`
    to the board's `sdkconfig.defaults.<target>` file.
  - Native USB OTG/CDC (ESP32-S2/S3): add `CONFIG_ESP_CONSOLE_USB_CDC=y` to the board's
    `sdkconfig.defaults.<target>` file.
- [ ] **LED GPIO and polarity**: note the GPIO number and whether the LED is active HIGH
  or active LOW. Update `CONFIG_BLINK_GPIO` (or equivalent Kconfig) and invert the
  drive logic if the new board differs from the default (active LOW).
- [ ] **LED type — WS2812 vs simple GPIO**: if the board-spec notes the LED is an
  addressable RGB (WS2812/SK6812), the firmware blink logic must be replaced with a
  single-pixel RMT write. Simple `gpio_set_level()` will not work.
- [ ] **ADC channel availability**: not required by this example (RSSI is obtained via
  the WiFi scan API, not ADC). No action needed, but note for future features.


## Roles

### Role 1 — SENSOR

- Wake from deep sleep every 60 seconds using the RTC timer.
- On wake: increment `RTC_DATA_ATTR uint16_t msg_count`.
- If the cached next-hop MAC (stored in RTC memory) is empty (all zeros), run
  the two-phase discovery window:
  1. Listen on the ESP-NOW channel for up to 2 seconds.
  2. On receiving a **gateway beacon** (`node_type == ESPNOW_NODE_GATEWAY`): record
     the sender MAC as the next-hop and unblock immediately.
  3. On receiving a **relay beacon** (`node_type == ESPNOW_NODE_RELAY`) during the
     wait window: save the sender MAC as a fallback candidate and keep listening for
     a gateway beacon.
  4. If the 2-second window expires with a gateway beacon received: use the gateway.
  5. If the window expires with only a relay beacon received: use the relay as the
     next hop and cache its MAC (with `node_type = ESPNOW_NODE_RELAY`).
  6. If neither beacon type was received: log a warning and return to deep sleep.
- Run an active WiFi scan targeting up to three SSIDs configured via Kconfig
  (`CONFIG_TARGET_SSID_1`, `CONFIG_TARGET_SSID_2`, `CONFIG_TARGET_SSID_3`).
  Record RSSI (dBm, int8_t) for each found SSID. Use `INT8_MIN` (–128) for SSIDs
  not found during the scan window.
- Deinitialize the WiFi scan and reinitialize ESP-NOW on the fixed channel.
- Send one ESP-NOW unicast packet to the cached next-hop MAC (gateway or relay).
  Set `hop_count = 0` — the relay increments it on forwarding.
- Log whether the packet is being sent directly to the gateway or via a relay node.
- Wait up to 200 ms for the send callback. On success: blink LED once. On failure:
  fast-blink LED, invalidate the next-hop MAC cache in RTC memory, and log a warning.
- Deinitialize ESP-NOW and stop WiFi cleanly before entering deep sleep.

### Role 2 — RELAY

- Never deep sleep. Run continuously in receive mode on the fixed ESP-NOW channel.
- On receiving any `espnow_payload_t` packet:
  - Check a static dedup table (node_id + msg_count pair). If already seen, drop silently.
  - Record the pair in the dedup table.
  - Increment `hop_count` by 1.
  - Forward the packet via ESP-NOW unicast to the cached gateway MAC.
  - Blink LED once.
- Listen for a **gateway beacon** (`node_type == ESPNOW_NODE_GATEWAY`) on startup and
  cache the gateway MAC. Ignore beacons with `node_type == ESPNOW_NODE_RELAY`.
- **After the gateway MAC is cached**, begin broadcasting its own relay beacon every
  `ESPNOW_BEACON_INTERVAL_MS` to the broadcast address (`FF:FF:FF:FF:FF:FF`). The relay
  beacon carries `node_type = ESPNOW_NODE_RELAY` so sensors can identify it and use it
  as a fallback next hop. A relay only broadcasts after it has a valid gateway path —
  this prevents sensors from routing through a relay that is itself disconnected.
- If the gateway is not found within the 2-second discovery window, continue in receive
  mode and start broadcasting once a gateway beacon arrives later.
- Log forwarded packets over serial (node_id, msg_count, hop_count, RSSI values).

### Role 3 — GATEWAY

- Never sleep. Run continuously in receive mode.
- On startup: broadcast a beacon packet every 5 seconds to the ESP-NOW broadcast
  address (`FF:FF:FF:FF:FF:FF`). The beacon carries only a type byte and sequence
  number; the gateway MAC is implicit from the sender address seen by the receiver.
- Register an `esp_now_recv_cb_t` callback. Accept packets from any MAC address.
  Do not require pre-registered peers in the receive path.
- Blink LED once per received packet.
- Display a real-time ANSI TUI dashboard over the serial monitor:
  - **Fixed header** (rows 1–5): title bar with channel, node ID, and MAC address;
    status line with beacon pulse indicator (`[*]` green for 2 s after each beacon,
    `[ ]` gray otherwise), beacon count, uptime, and total packet count; separator
    lines; column headers labelled with the three configured SSID names.
  - **Scroll region** (row 6 and below): one line per received packet, appended and
    scrolled automatically. Each line shows arrival timestamp, role and path
    (`[S/direct]`, `[SR/relayed]`, etc.), node ID, sequence number, hop count, and
    RSSI for each configured SSID.
  - **Color coding**: sensor role lines in green, sensor-relay in yellow; RSSI values
    in green (> −60 dBm), yellow (> −80 dBm), red (≤ −80 dBm), or gray (not found).
  - **Hidden cursor**: terminal cursor is hidden for the lifetime of the monitor
    session so the fixed header rows appear fully static.
  - All screen output is serialized through the main task. The beacon timer callback
    and the ESP-NOW receive callback only update shared state and post a FreeRTOS
    task notification; they never write to the terminal directly.

### Role 4 — SENSOR_RELAY

- Initialize WiFi in STA mode and ESP-NOW on the fixed channel.
- Configure ESP-IDF power management (`CONFIG_PM_ENABLE` + tickless idle) for light sleep.
  The radio remains in periodic listen mode during light sleep; ESP-NOW receive callbacks
  fire normally when a packet arrives, waking the CPU for the duration of the callback.
- **Relay behaviour** (identical to Role 2):
  - Register recv_cb. On receiving `espnow_payload_t`: dedup check, increment hop_count,
    forward to cached gateway MAC, notify LED task.
  - Listen for **gateway beacon** (`node_type == ESPNOW_NODE_GATEWAY`) on startup; cache
    gateway MAC. Ignore beacons with `node_type == ESPNOW_NODE_RELAY`.
  - **After the gateway MAC is cached**, broadcast its own relay beacon every
    `ESPNOW_BEACON_INTERVAL_MS` (same logic as the RELAY role) so sensor nodes can
    route through this node when the gateway is out of direct range.
- **Sensor behaviour** (timer-driven, not deep-sleep-driven):
  - Start a periodic `esp_timer` firing every `CONFIG_SENSOR_RELAY_SCAN_INTERVAL_SEC`
    seconds.
  - On timer fire: unregister recv_cb, deinit ESP-NOW, run WiFi RSSI scan (same logic as
    sensor role), reinit ESP-NOW on fixed channel, send `espnow_payload_t` to gateway,
    re-register recv_cb. Resume light sleep.
  - Increment `msg_count` in a static (non-RTC) variable — no deep sleep, so RTC memory
    is not needed.
  - Gateway MAC is stored in a static variable (not RTC) for the same reason.
- On discovery timeout at startup: continue in relay-only mode; next timer fire retries
  the send. Log a warning. Relay beacon starts as soon as gateway MAC is first seen.
- The relay function is unavailable during the ~2–3 s scan+send window each cycle.
  At a 60 s interval this is < 5% downtime — acceptable for a mesh.

## Node Identity

Each node derives its `node_id` at runtime from its factory-burned WiFi STA MAC address
(last byte, `mac[5]`). This means all boards flashed with the same role firmware are
identical — no per-device build or provisioning step is required. Each board self-
identifies uniquely because every ESP32 MAC is globally unique.

On every boot, the node logs its derived `node_id` alongside its full MAC address so
that operators can cross-reference node IDs to physical boards from the serial output
alone. The gateway's receive callback also carries the source MAC, which is logged with
every received packet for the same purpose.

If `mac[5]` is `0x00` or `0xFF` (pathological values that should never occur on
production silicon), the node falls back to `mac[4]` as the identifier.

## Deploying Multiple Nodes

**One firmware binary per role. No per-device compilation.**

1. **Build once per role** — four total builds: gateway, relay, sensor, sensor-relay.
   All boards of the same role receive the same binary.
2. **Flash gateway first** — power it on before any other node. It must be broadcasting
   its beacon before sensors and relays attempt discovery.
3. **Flash relays** — all relay boards receive the identical relay binary. Power on
   after the gateway.
4. **Flash sensors** — all sensor boards receive the identical sensor binary. Power on
   last, or at any time after the gateway is running.
5. **Identify each device** — on boot, every node prints:
   `node_id=NN mac=AA:BB:CC:DD:EE:FF role=SENSOR`. The gateway log includes the source
   MAC on every received packet. Use these to build a `node_id → physical board` map
   for your deployment.
6. **Shared configuration** — `ESPNOW_CHANNEL`, `SENSOR_SLEEP_SEC`, and `TARGET_SSID_*`
   are the same across all nodes of a given role and are baked into the role firmware at
   build time. No per-device configuration is needed.

## Shared Payload Struct

```c
typedef struct __attribute__((packed)) {
    uint8_t  node_id;    /* last byte of WiFi STA MAC (runtime)          */
    uint16_t msg_count;  /* monotonic counter, RTC_DATA_ATTR             */
    uint8_t  hop_count;  /* 0 = direct to gateway, 1+ = relayed         */
    int8_t   rssi[3];    /* RSSI per configured SSID; INT8_MIN=none      */
    uint8_t  role;       /* ESPNOW_ROLE_SENSOR=0, ESPNOW_ROLE_SENSOR_RELAY=1 */
} espnow_payload_t;      /* 8 bytes — well under 250-byte ESP-NOW limit */
```

## Hardware Dependencies

- Board-spec: `board-specs/seeed/xiao-esp32s3.md` (ESP32-S3, LED GPIO 21 active LOW)
- No external components required.
- Minimum deployment for full mesh demonstration: 3 × XIAO ESP32S3 boards
  (1 gateway, 1 relay, 1 sensor). A sensor-relay can substitute for a separate relay
  and sensor pair: 2-board deployment with 1 gateway + 1 sensor-relay also demonstrates
  the full architecture.
- 2-board deployment (gateway + sensor, no relay) is valid for verifying the direct
  delivery path.
- One or more real WiFi access points must be present in the environment for RSSI
  readings to be non-empty. The example works without any AP (all RSSI = INT8_MIN)
  but the sensor data will be uninformative.

## Power Budget (sensor node)

| State | Current | Duration | Notes |
| --- | --- | --- | --- |
| Deep sleep | ~14 µA | ~59.4 s | XIAO ESP32S3 deep sleep floor |
| WiFi init + active scan | ~80 mA | ~300 ms | Targeted SSID scan, all channels |
| ESP-NOW init + TX + teardown | ~50 mA | ~200 ms | Includes peer registration |
| LED blink | ~10 mA | ~20 ms | Single blink on success |
| Active window total | ~67 mA avg | ≤ 600 ms | From wake to sleep entry |
| **Target average** | **< 1 mA** | — | Over full 60-second cycle |

> Power calculation: `(67 mA × 0.6 s + 0.014 mA × 59.4 s) / 60 s ≈ 0.68 mA`.
> The WiFi scan is the dominant cost. Reducing configured SSIDs or limiting scan
> channels in `sdkconfig.defaults` can push average below 500 µA.

## Power Budget (sensor-relay node)

| State | Current | Duration | Notes |
| --- | --- | --- | --- |
| Light sleep (relaying) | ~1–5 mA | ~57–59 s | Radio listens on DTIM beacon interval |
| WiFi scan | ~80 mA | ~300 ms | Every `SENSOR_RELAY_SCAN_INTERVAL_SEC` |
| ESP-NOW TX + teardown | ~50 mA | ~200 ms | Per scan cycle |
| LED blink (relay fwd) | ~10 mA | ~20 ms | Per forwarded packet |
| **Target average** | **~3–8 mA** | — | Dominated by light sleep radio listen |

> Significantly higher than a deep-sleeping pure sensor (~0.68 mA avg), but an order of
> magnitude lower than an always-on relay (~80–150 mA). The right choice when a node
> must simultaneously extend mesh coverage and report its own measurements.

## Limitations and Topology Characterization

This example implements a **tree topology with dynamic discovery and partial recovery**
— not a self-healing mesh. Understanding the distinction prevents over-claiming the
network's capabilities when deploying or presenting this example.

### What this example is NOT

| Claim | Reality |
| --- | --- |
| "Self-healing mesh" | No. There is no alternate-path selection. If the relay between a sensor and the gateway fails, the sensor has no way to find a different relay. It fails until the original relay recovers or the sensor regains direct gateway range. |
| "Multi-hop mesh" | Partially. Packets traverse at most one relay hop in the current implementation. A sensor cannot route through two relays in sequence. |
| "Fault-tolerant" | Partially. The gateway is a single point of failure. If the gateway goes down, all nodes lose their destination. There is no backup gateway. |
| "Mesh routing" | No. Nodes do not exchange routing tables, path metrics, or topology advertisements. Each node knows only its single cached next hop. |

### Specific shortcomings

1. **Single gateway, no failover** — all paths terminate at one gateway. A gateway reboot
   is survivable (sensors and relays rediscover via beacon) but a gateway failure with
   no recovery means the network is dark until the gateway is restored.

2. **Single relay path, no alternate selection** — sensors cache one next-hop MAC. If
   that relay fails, the sensor has no fallback (unless it can reach the gateway
   directly). The dedup table prevents relay storms but does nothing to select a
   better path.

3. **One relay hop maximum** — the forwarding logic in the relay and sensor-relay roles
   sends the packet directly to the gateway MAC. A relay never forwards to another
   relay, so the network depth is capped at: sensor → relay → gateway.

4. **No path quality metric** — during discovery, a sensor accepts the first beacon it
   hears (gateway preferred, relay as fallback). It does not compare RSSI or hop
   distance to choose a better relay when multiple relays are in range.

5. **No topology advertisement** — relay beacons carry only a node type byte and
   sequence number. Downstream nodes cannot determine distance-to-gateway or relay
   load from beacon content alone.

### How to extend toward a true self-healing mesh

These extensions are not implemented but follow naturally from the existing architecture:

| Extension | Approach |
| --- | --- |
| **Multiple relay candidates** | Store up to N relay MACs ranked by beacon RSSI. On primary relay failure, promote the next candidate without waiting for a full discovery window. |
| **Relay-to-relay forwarding** | Add a "distance to gateway" field to relay beacons. A relay that receives a packet from another relay forwards it if its own distance is smaller. Prevents loops; enables depth > 1. |
| **Backup gateway** | Broadcast beacon from two gateway nodes on the same channel. Sensors and relays cache both MACs and fail over when the primary stops beaconing. |
| **Path quality selection** | During the discovery window, collect all relay beacons. After the window closes, select the relay with the strongest RSSI rather than the first heard. |
| **Production mesh stack** | For deployments requiring true self-healing, replace this example with ESP-IDF's `ESP-WIFI-MESH` component, which implements automatic tree restructuring, multi-hop routing, and root failover natively. |

## Success Criteria

- Gateway receives a packet from each sensor node on every 60-second wake cycle.
- `msg_count` increments monotonically across received packets from any given node_id.
- Packets routed through a relay node arrive at the gateway with `hop_count ≥ 1`.
- No duplicate packets arrive at the gateway for the same (node_id, msg_count) pair.
- RSSI values for configured SSIDs that are present in the RF environment fall within
  the –30 dBm to –90 dBm range. Absent SSIDs report `INT8_MIN`.
- Sensor active window (wake to sleep) is ≤ 600 ms per serial timestamps.
- Sensor returns to deep sleep within 600 ms of wake even when no gateway is reachable.
- Gateway MAC is not required as a compile-time constant; it is discovered via beacon.
- SENSOR_RELAY node forwards packets from other sensors (gateway sees hop_count ≥ 1) AND
  sends its own payloads with hop_count=0 on the configured interval.
- Serial monitor on a SENSOR_RELAY node remains connected continuously — no deep-sleep
  disconnect events.
- Relay and sensor-relay nodes do not broadcast their own beacons until they have a
  valid gateway MAC — preventing sensors from routing through a disconnected relay.
- When gateway is out of direct range but a relay is reachable, the sensor discovers
  the relay via its relay beacon and routes through it. Gateway receives the packet
  with `hop_count = 1`.
- When both gateway and relay beacons are received during the discovery window, the
  sensor prefers the gateway beacon and sends directly (`hop_count = 0`).
