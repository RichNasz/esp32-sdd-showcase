# ==================== BOARD SELECTION ====================
# Change only this section, then regenerate the project
#
# Board: seeed-xiao-esp32s3          ← ACTIVE / DEFAULT
#
# =======================================================
# Institutional Memory: shared-specs/AIGenLessonsLearned.md — read before regenerating

# Coding Specification — ESP-NOW Low-Power Mesh with WiFi RSSI Sensing

## Architecture

Compile-time three-role firmware. A single Kconfig choice (`NODE_ROLE`) selects
`SENSOR`, `RELAY`, or `GATEWAY` at build time — not at runtime. All three roles share
the same payload struct definition and the same WiFi + ESP-NOW initialization pattern.

**Sensor** nodes wake from deep sleep, run a WiFi scan to collect per-SSID RSSI data,
stop the scan driver, then initialize ESP-NOW on the fixed channel to transmit one
packet. The critical sequencing constraint is that the scan driver must be fully stopped
before ESP-NOW is initialized — both consume the same radio hardware and cannot be
active simultaneously.

**Relay** nodes run continuously in receive mode. On receiving a packet whose
(node_id, msg_count) pair has not been seen before, they increment the hop count and
forward the packet unicast to the gateway. A fixed-size circular dedup table in static
memory prevents relay storms when a packet is overheard by multiple relays.

**Gateway** nodes run continuously. They broadcast a small beacon packet to the ESP-NOW
broadcast address at a fixed interval so that sensor and relay nodes can discover the
gateway MAC address at runtime — no compile-time MAC hardcoding is required. They also
register a receive callback that logs all incoming packets, including hop count and
per-SSID RSSI values.

## Gateway Discovery

Sensor and relay nodes store the gateway MAC in RTC-retained memory so it survives deep
sleep cycles. On first boot, or after a send failure invalidates the cache, the node
enters a brief discovery window in which it listens for a gateway beacon. The gateway
MAC is read from the sender address carried in the receive callback — the beacon payload
itself only needs a type discriminator and a sequence number. If no beacon arrives within
the discovery window, the node logs a warning and returns to sleep rather than blocking
indefinitely.

## Packet Routing and Dedup

The relay dedup table is a small circular buffer in static memory (not RTC memory —
it resets on reboot). Each entry holds a (node_id, msg_count) pair. Before forwarding,
the relay scans the table; a match causes a silent drop. On a miss, the relay writes the
pair to the next slot, increments hop_count, and forwards to the gateway. The table size
should be large enough that entries are not evicted before their TTL in the network
expires — sixteen entries is a reasonable starting point given the 60-second sensor cycle.

## Key Constraints

- The WiFi scan driver and ESP-NOW cannot be active simultaneously. The scan must
  complete and the scan driver must be stopped before ESP-NOW initialization. If these
  overlap, ESP-NOW initialization will fail.
- All nodes must operate on the same channel. Configure a fixed channel via Kconfig
  (`CONFIG_ESPNOW_CHANNEL`, default 1) and set it explicitly after WiFi start.
  Channel mismatch is the most common silent failure in ESP-NOW networks.
- The payload struct must be declared `__attribute__((packed))` and validated with a
  compile-time assertion against the 250-byte ESP-NOW maximum payload limit.
- Sensor sleep duration is configurable via Kconfig (`CONFIG_SENSOR_SLEEP_SEC`,
  default 60 seconds).
- Send callback timeout is 200 ms. On expiry, log a warning, invalidate the gateway
  MAC cache, and proceed to sleep. Never block indefinitely.
- All configurable parameters (node ID, target SSIDs, ESP-NOW channel, sleep duration)
  must be exposed via Kconfig in `main/Kconfig.projbuild`.

## WiFi Scan Strategy

The sensor node runs an active WiFi scan for the configured SSIDs before initializing
ESP-NOW. The scan should target all channels with a short per-channel dwell time
(around 100 ms maximum) to keep the total scan window within budget. After the scan,
the driver processes the returned AP records and matches each configured SSID string
against the results. If a configured SSID appears multiple times (multiple BSSIDs),
use the record with the highest RSSI. Unmatched SSID slots in the payload are set to
`INT8_MIN` (–128 dBm), which serves as the sentinel value for "not found".

The scan must be explicitly stopped and the channel reset to `CONFIG_ESPNOW_CHANNEL`
before ESP-NOW initialization. Leaving the radio on whatever channel the scan last
visited will break ESP-NOW delivery for any node operating on a different channel.

## Per-Role sdkconfig.defaults Files

Generate three sdkconfig files so that each role can be built cleanly without modifying
source files:

- `sdkconfig.defaults` — common settings plus sensor role (`CONFIG_NODE_ROLE_SENSOR=y`).
  This is the default build used by automated tests.
- `sdkconfig.defaults.relay` — relay role override only.
- `sdkconfig.defaults.gateway` — gateway role override only.

The `SDKCONFIG_DEFAULTS` environment variable accepted by idf.py supports semicolon-
separated lists, allowing the base file and a role override to be layered without
editing either.

## Preferred Libraries and APIs

Use the built-in ESP-NOW API. It operates below the IP stack and requires no AP
association, DHCP, or TCP — ideal for low-power peer-to-peer messaging where full
WiFi connectivity would be excessive overhead.

Use a FreeRTOS binary semaphore to synchronize the ESP-NOW send callback with the
main task. This avoids busy-waiting and prevents the race condition that a global
`volatile bool` flag introduces when the callback fires before the wait starts.

Use a FreeRTOS task notification to drive the gateway and relay LED blinks from a
dedicated low-priority task. The receive callback must not be delayed by LED timing.

## Non-Functional Requirements

- Sensor must call `gpio_hold_dis()` on the LED GPIO before driving it after waking
  from deep sleep. Pad state may be held from the previous cycle.
- Relay and gateway LED blinks must be non-blocking. The receive callback must return
  as fast as possible — never block in a receive callback.
- `esp_now_send()` failure must be logged; never use `ESP_ERROR_CHECK` on it. A failed
  send is a recoverable event and must not abort the device.
- Relay must not hold any FreeRTOS semaphore or mutex across the receive callback.
  Callbacks execute in a system task context with limited stack.

## Gotchas

- **WiFi scan before ESP-NOW**: calling `esp_now_init()` while a WiFi scan is active
  will fail. The scan must be fully stopped first.
- **Channel must be set explicitly**: after WiFi start, set the channel to
  `CONFIG_ESPNOW_CHANNEL` before any ESP-NOW operation. Do not assume the channel
  remains at the desired value after a scan.
- **Peer registration before send**: `esp_now_send()` to an unregistered peer returns
  an error that is easy to confuse with a channel mismatch. Register all destination
  MACs before sending — including the broadcast address used for gateway beacons.
- **WIFI_MODE_STA required**: `WIFI_MODE_NULL` is insufficient for ESP-NOW on
  ESP32-S3, even when no AP association is made.
- **nvs_flash_init() before WiFi**: omitting this call causes WiFi init to fail
  because PHY calibration data cannot be stored. See `shared-specs/AIGenLessonsLearned.md`.
- **RTC memory initialization**: on power-on reset, explicitly validate the RTC-stored
  gateway MAC with a known magic number stored alongside it, rather than assuming
  all-zeros means "not discovered". On a warm reset the cache may contain stale data.

## File Layout (non-standard files)

- `main/Kconfig.projbuild` — `NODE_ROLE` choice (SENSOR / RELAY / GATEWAY),
  `ESPNOW_CHANNEL` integer (default 1), `SENSOR_SLEEP_SEC` integer (default 60),
  `TARGET_SSID_1` / `TARGET_SSID_2` / `TARGET_SSID_3` strings (up to 32 chars,
  empty string means disabled), `NODE_ID` integer (1–254, default 1).
- `sdkconfig.defaults` — common + sensor role defaults.
- `sdkconfig.defaults.relay` — relay role override.
- `sdkconfig.defaults.gateway` — gateway role override.
