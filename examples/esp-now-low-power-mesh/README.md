<!-- ================================================
     AGENT-GENERATED — DO NOT EDIT BY HAND
     Generated from specs/ using esp32-sdd-documentation-generator skill
     Date: 2026-04-07 | Agent: Claude Code
     ================================================ -->

# ESP-NOW Low-Power Mesh with WiFi RSSI Sensing

> **Real multi-hop mesh networking.** Sensor nodes wake from deep sleep, scan nearby WiFi access points for real signal-strength data, then route ESP-NOW packets to the gateway through one or more relay hops — with dynamic gateway discovery and no hardcoded MAC addresses.

## Overview

Three roles run on the same firmware, selected at build time via a Kconfig choice:
**sensor**, **relay**, and **gateway**.

**Sensor nodes** wake every 60 seconds, run an active WiFi scan to record the signal
strength (RSSI in dBm) of up to three configured SSIDs, then initialize ESP-NOW on a
fixed channel and transmit one compact packet to the gateway. If the gateway MAC is not
yet cached in RTC memory, the sensor first listens for a gateway beacon to discover it
dynamically. After transmitting (or timing out), the sensor tears down the radio fully
and re-enters deep sleep.

**Relay nodes** run continuously in receive mode. When they receive a packet with a
(node\_id, msg\_count) pair not seen before, they increment the hop count and forward
the packet unicast to the gateway. A 16-entry circular dedup table prevents relay storms.

**Gateway nodes** run continuously. They broadcast a small beacon every 5 seconds so
sensor and relay nodes can discover the gateway MAC at runtime — no compile-time address
is required. When packets arrive, the gateway logs node\_id, msg\_count, hop\_count,
and the per-SSID RSSI values.

The RSSI data is genuinely useful: deploy multiple sensor nodes in different locations
and the gateway builds a real-time RF signal map of your environment.

## Supported Boards

| Role | Board | LED GPIO | Active LOW? |
| --- | --- | --- | --- |
| SENSOR / RELAY / GATEWAY | Seeed XIAO ESP32S3 | GPIO 21 | Yes |

All three roles use the same board and firmware image. Minimum deployment for a full
mesh demonstration is three boards (one per role). A two-board deployment (gateway +
sensor) demonstrates the direct delivery path without relaying.

## Build & Flash

```sh
cd examples/esp-now-low-power-mesh
idf.py set-target esp32s3
```

**Sensor role (default):**

```sh
idf.py build flash monitor
```

**Relay role:**

```sh
SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.defaults.relay" \
  idf.py fullclean build flash monitor
```

**Gateway role:**

```sh
SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.defaults.gateway" \
  idf.py fullclean build flash monitor
```

> Flash the gateway first. Sensor and relay nodes discover the gateway MAC automatically
> by listening for its beacon on startup — no `GATEWAY_MAC` Kconfig entry is required.

### Configuring Target SSIDs and Node ID

Open `idf.py menuconfig` → **ESP-NOW Mesh Configuration**:

- `TARGET_SSID_1` / `TARGET_SSID_2` / `TARGET_SSID_3` — SSIDs to scan for. Empty string
  disables that slot. At least one known AP should be configured for useful RSSI readings.
- `NODE_ID` — unique identifier per sensor or relay node (1–254).
- `ESPNOW_CHANNEL` — WiFi channel shared by all nodes (default: 1).
- `SENSOR_SLEEP_SEC` — sleep duration in seconds (default: 60).

## Packet Payload

```c
typedef struct __attribute__((packed)) {
    uint8_t  node_id;    /* sender ID, 1-254 (Kconfig)              */
    uint16_t msg_count;  /* monotonic counter, RTC_DATA_ATTR         */
    uint8_t  hop_count;  /* 0 = direct to gateway, 1+ = relayed     */
    int8_t   rssi[3];    /* RSSI per SSID slot (dBm); -128 = none   */
    uint8_t  _pad;       /* reserved                                 */
} espnow_payload_t;      /* 8 bytes — well within 250-byte limit     */
```

A compile-time `static_assert` enforces `sizeof(espnow_payload_t) <= 250`.

## Expected Serial Output

**Gateway:**

```
I (312) gw: Beacon #1 broadcast on channel 1
I (60412) gw: RX node_id=1 msg_count=1 hop_count=0 rssi=[-52, -67, -128]
I (120412) gw: RX node_id=1 msg_count=2 hop_count=0 rssi=[-51, -68, -128]
I (120580) gw: RX node_id=2 msg_count=4 hop_count=1 rssi=[-74, -128, -128]
```

**Sensor:**

```
I (203) sensor: Wake -- msg_count=3
I (320) sensor: Gateway discovered: AA:BB:CC:DD:EE:FF
I (520) sensor: WiFi scan done -- rssi=[-52, -67, -128]
I (680) sensor: ESP-NOW sent OK (hop_count=0)
I (690) sensor: Entering deep sleep (60 s)
```

**Relay:**

```
I (215) relay: Gateway discovered: AA:BB:CC:DD:EE:FF
I (60220) relay: FWD node_id=2 msg_count=4 hop_count=1
```

## Key Concepts

- **WiFi scan before ESP-NOW**: both use the same radio. The scan driver must be fully
  stopped before `esp_now_init()` is called — initializing ESP-NOW during an active scan
  will fail.
- **Dynamic gateway discovery**: the gateway broadcasts a small beacon every 5 seconds.
  Sensor and relay nodes cache the gateway MAC in `RTC_DATA_ATTR` memory, so it survives
  deep sleep and is reused without re-discovery on subsequent wake cycles.
- **Multi-hop routing via hop\_count**: each relay increments `hop_count` before
  forwarding. The gateway log shows exactly how many hops each packet traversed.
- **Relay dedup table**: a 16-entry circular buffer keyed on (node\_id, msg\_count)
  prevents the same packet being forwarded multiple times when a node is in range of
  multiple relays simultaneously.
- **Real sensor data (WiFi RSSI)**: the `rssi[]` values are real measurements of the
  RF environment in dBm — not synthetic placeholders. `INT8_MIN` (–128) means the SSID
  was not found during the scan window.
- **Channel discipline**: all nodes share a fixed channel (`CONFIG_ESPNOW_CHANNEL`).
  Channel mismatch is the most common silent failure in ESP-NOW deployments.
- **Radio teardown before sleep**: sensor nodes deinitialize ESP-NOW and stop WiFi before
  entering deep sleep. Leaving the radio active defeats the power budget.
- `gpio_hold_dis()` must be called on the LED GPIO before driving it after a deep sleep
  wakeup — pad state may have been held from the previous cycle.
- `nvs_flash_init()` must be called before any WiFi API (PHY calibration requires NVS).

## Power Budget (sensor node)

| State | Current | Duration | Notes |
| --- | --- | --- | --- |
| Deep sleep | ~14 µA | ~59.4 s | XIAO ESP32S3 sleep floor |
| WiFi init + active scan | ~80 mA | ~300 ms | Targeted SSID scan, all channels |
| ESP-NOW init + TX + teardown | ~50 mA | ~200 ms | Peer registration included |
| LED blink | ~10 mA | ~20 ms | Single blink on success |
| Active window total | ~67 mA avg | ≤ 600 ms | Wake to sleep entry |
| **Target average** | **< 1 mA** | — | Over full 60-second cycle |

> Calculation: `(67 mA × 0.6 s + 0.014 mA × 59.4 s) / 60 s ≈ 0.68 mA`. The WiFi scan
> is the dominant cost. Shortening `SENSOR_SLEEP_SEC` increases freshness at the cost
> of higher average current.

## Testing

Testing philosophy: automated first, manual only when hardware observation is unavoidable.

### Automated Tests

1. **T-A1 — Zero-Warning Build (Sensor Role)**

   ```sh
   cd examples/esp-now-low-power-mesh
   idf.py set-target esp32s3 && idf.py build
   ```

   Pass: exit code 0, zero compiler warnings. Default `sdkconfig.defaults` selects
   the sensor role.

2. **T-A2 — Zero-Warning Build (Relay Role)**

   ```sh
   SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.defaults.relay" \
     idf.py fullclean build
   ```

   Pass: exit code 0, zero compiler warnings for the relay role.

3. **T-A3 — Zero-Warning Build (Gateway Role)**

   ```sh
   SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.defaults.gateway" \
     idf.py fullclean build
   ```

   Pass: exit code 0, zero compiler warnings for the gateway role.

4. **T-A4 — Payload Size Compile-Time Assert**

   Covered by T-A1. The source contains `static_assert(sizeof(espnow_payload_t) <= 250)`.
   A successful build confirms the struct is within the ESP-NOW 250-byte payload limit.

### Manual Tests — hardware required

**T-M1 — Direct Path: Sensor to Gateway (2 boards)**

Why manual: real ESP-NOW packet exchange over the air requires two physical boards.

Requires: 2× XIAO ESP32S3 + at least one real WiFi access point in range.

1. Flash board A as gateway. Note its MAC from the startup serial log.
2. Set `CONFIG_TARGET_SSID_1` to a known AP SSID, set `CONFIG_NODE_ID=1`,
   flash board B as sensor.
3. Power on gateway first; confirm beacon broadcast log every 5 seconds.
4. Power on sensor; confirm it logs "Gateway discovered" after beacon reception.
5. Watch gateway serial for 5 consecutive receive lines from node\_id=1.
6. Confirm `msg_count` increments by 1 per packet and `hop_count=0` (direct path).
7. Confirm at least one `rssi[]` value is between –30 and –90 dBm.

Pass: 5/5 packets received; msg\_count monotonically increases; hop\_count=0;
at least one RSSI value in –30 to –90 dBm range.

---

**T-M2 — Relayed Path: Sensor through Relay to Gateway (3 boards)**

Why manual: multi-hop routing requires three boards and physical separation to force
the sensor outside direct range of the gateway.

Requires: 3× XIAO ESP32S3. Place relay between sensor and gateway (e.g. different
rooms) so the sensor cannot reach the gateway directly.

1. Flash board A as gateway, board B as relay, board C as sensor (`NODE_ID=2`).
2. Power on gateway, then relay, then sensor.
3. Confirm relay logs "Gateway discovered" and enters receive mode.
4. Watch gateway for 5 consecutive packets from node\_id=2 with `hop_count=1`.
5. Confirm relay serial shows forwarded packets with incremented hop\_count.

Pass: 5/5 packets at gateway with hop\_count=1; relay log confirms forwarding.

---

**T-M3 — Relay Dedup: No Duplicate Delivery**

Why manual: requires observing a packet heard by both relay and gateway arriving only
once — not reproducible without physical multi-path RF conditions.

Requires: 3× XIAO ESP32S3, sensor within range of both relay and gateway simultaneously.

1. Flash all three boards as in T-M2 but place sensor within range of both relay and
   gateway.
2. Let sensor transmit 5 packets.
3. Confirm gateway receives exactly 5 unique (node\_id, msg\_count) pairs — not 10.

Pass: exactly 5 packets received at gateway with no duplicates.

---

**T-M4 — RSSI Plausibility**

Why manual: requires a real WiFi access point and physical distance variation to verify
that RSSI values respond to range.

Requires: 1× XIAO ESP32S3 (sensor role) + a known WiFi AP.

1. Set `CONFIG_TARGET_SSID_1` to a known AP SSID. Flash as sensor and open monitor.
2. On first wake, confirm a logged RSSI value in –30 to –90 dBm.
3. Move the board farther from the AP; confirm RSSI decreases (more negative).

Pass: RSSI in –30 to –90 dBm at close range; measurably lower at greater distance.

---

**T-M5 — Sensor Active Window ≤ 600 ms**

Why manual: precise wake-to-sleep timing requires serial timestamp observation on hardware.

1. Open sensor serial monitor.
2. Record timestamp of "Wake" log line and "Entering deep sleep" log line.
3. Difference must be ≤ 600 ms across at least 5 consecutive cycles.

Pass: active window ≤ 600 ms per serial timestamps on all 5 cycles.

---

**T-M6 — Sensor Sleeps When No Gateway Is Reachable**

Why manual: requires physical absence of the gateway board to exercise the timeout path.

1. Flash sensor. Do not power on any gateway.
2. Confirm sensor wakes, listens for beacon, logs "no gateway discovered", and enters
   deep sleep within 600 ms of wake.
3. Confirm no blocking hang.

Pass: sensor returns to deep sleep within 600 ms with no gateway present.

## Spec Files

- [specs/FunctionalSpec.md](specs/FunctionalSpec.md)
- [specs/CodingSpec.md](specs/CodingSpec.md)
- [specs/TestSpec.md](specs/TestSpec.md)
