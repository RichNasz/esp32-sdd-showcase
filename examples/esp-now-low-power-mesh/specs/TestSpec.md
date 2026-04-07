# Testing Specification — ESP-NOW Low-Power Mesh with WiFi RSSI Sensing

## Testing Philosophy

Automated testing using native ESP-IDF tools is strongly preferred.
Additional tools are only allowed with explicit justification and documented benefits.
Manual testing is used only when no practical automated alternative exists.

---

## Automated Tests

### T-A1 — Zero-Warning Build (Sensor Role)

**Tool**: `idf.py` (native)

```sh
cd examples/esp-now-low-power-mesh
idf.py set-target esp32s3 && idf.py build
```

Pass: exit code 0, zero compiler warnings. Default `sdkconfig.defaults` selects sensor role.

---

### T-A2 — Zero-Warning Build (Relay Role)

**Tool**: `idf.py` (native)

```sh
cd examples/esp-now-low-power-mesh
SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.defaults.relay" idf.py fullclean build
```

Pass: exit code 0, zero compiler warnings for relay role.

---

### T-A3 — Zero-Warning Build (Gateway Role)

**Tool**: `idf.py` (native)

```sh
cd examples/esp-now-low-power-mesh
SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.defaults.gateway" idf.py fullclean build
```

Pass: exit code 0, zero compiler warnings for gateway role.

---

### T-A4 — Payload Size Compile-Time Assert

**Tool**: `idf.py` (native — `static_assert` fires at build time)

Covered by T-A1. The source file contains:

```c
static_assert(sizeof(espnow_payload_t) <= 250,
              "espnow_payload_t exceeds ESP-NOW 250-byte maximum");
```

Pass: T-A1 build succeeds, confirming `sizeof(espnow_payload_t) <= 250`.

---

## Manual Tests

*ESP-NOW packet exchange and real RSSI values require physical boards and a real RF
environment. Inter-device radio communication and live WiFi scanning cannot be
simulated without multi-device hardware-in-the-loop tooling.*

---

### T-M1 — Direct Path: Sensor to Gateway (2 boards)

**Why manual**: Real ESP-NOW packet exchange over the air requires two XIAO ESP32S3 boards.

Hardware: 2× XIAO ESP32S3. One or more real WiFi access points in range.

Setup:

1. Build and flash board A as gateway (use `sdkconfig.defaults.gateway` override). Note
   its MAC address from the serial log on startup.
2. Configure `CONFIG_TARGET_SSID_1` (and optionally SSID 2/3) to match known APs in
   the environment. Set `CONFIG_NODE_ID=1`. Build and flash board B as sensor.

Steps:

1. Power on gateway (board A) first. Confirm beacon broadcast log every 5 seconds.
2. Power on sensor (board B). Confirm it logs "gateway discovered" after beacon reception.
3. Watch gateway serial monitor for 5 consecutive receive log lines from node_id=1.
4. Confirm `msg_count` increments by 1 on each packet (no drops).
5. Confirm `hop_count = 0` on all received packets (direct path).
6. Confirm at least one `rssi[]` value is in the range –30 to –90 dBm for a known AP.

Pass: 5/5 packets received; msg_count monotonically increases; hop_count=0; at least one
RSSI value is non-INT8_MIN and within –30 to –90 dBm.

---

### T-M2 — Relayed Path: Sensor through Relay to Gateway (3 boards)

**Why manual**: Multi-hop routing requires three boards and physical separation to force
the sensor out of direct range of the gateway.

Hardware: 3× XIAO ESP32S3. Arrange boards so that the relay is physically between sensor
and gateway, with the sensor unable to reach the gateway directly (e.g. different rooms,
or attenuate with a metal enclosure).

Setup:

1. Flash board A as gateway. Flash board B as relay. Flash board C as sensor
   (`CONFIG_NODE_ID=2`).
2. Power on gateway, then relay, then sensor.

Steps:

1. Confirm gateway logs beacon every 5 seconds.
2. Confirm relay logs "gateway discovered" and enters receive mode.
3. Confirm sensor logs "gateway discovered" via relay beacon reception or direct beacon.
4. Watch gateway for 5 consecutive packets from node_id=2.
5. Confirm `hop_count = 1` on received packets (one relay hop).
6. Confirm relay serial log shows forwarded packets with incremented hop_count.

Pass: 5/5 packets received at gateway with hop_count=1; relay logs confirm forwarding.

---

### T-M3 — Relay Dedup: No Duplicate Delivery

**Why manual**: Requires observing that a packet arriving via two paths (direct + relay)
is delivered exactly once at the gateway.

Hardware: 3× XIAO ESP32S3, sensor within range of both relay and gateway simultaneously.

Steps:

1. Flash all three boards as in T-M2 but place sensor within range of both relay and
   gateway.
2. Let sensor transmit 5 packets.
3. Confirm gateway receives exactly 5 packets total (not 10 — no duplicates from relay).

Pass: exactly 5 unique (node_id, msg_count) pairs received at gateway.

---

### T-M4 — RSSI Plausibility

**Why manual**: Requires a real WiFi AP in the environment.

Hardware: 1× XIAO ESP32S3 (sensor role) + any WiFi AP with known SSID.

Steps:

1. Configure `CONFIG_TARGET_SSID_1` to match a known AP in the environment.
2. Flash as sensor, connect a serial monitor.
3. On first wake, confirm sensor logs RSSI for the configured SSID.
4. Confirm the value is in the range –30 to –90 dBm.
5. Move board to a more distant location and confirm RSSI decreases (more negative).

Pass: logged RSSI is in –30 to –90 dBm range at close range; decreases with distance.

---

### T-M5 — Sensor Active Window ≤ 600 ms

**Why manual**: Precise sleep/wake timing requires serial timestamp observation on real hardware.

Hardware: 1× XIAO ESP32S3 (sensor role) + 1× gateway.

Steps:

1. Open sensor serial monitor.
2. Record timestamp of "wake" log line and "entering deep sleep" log line.
3. Difference must be ≤ 600 ms.

Pass: active window ≤ 600 ms per serial timestamps on at least 5 consecutive cycles.

---

### T-M6 — Sensor Sleeps When No Gateway Is Reachable

**Why manual**: Requires physical absence of the gateway to test the timeout path.

Hardware: 1× XIAO ESP32S3 (sensor role), no gateway powered on.

Steps:

1. Flash sensor. Do not power on any gateway board.
2. Monitor sensor serial output.
3. Confirm sensor wakes, listens for beacon, logs "no gateway discovered", then enters
   deep sleep.
4. Confirm no blocking hang. Confirm sleep entry within 600 ms of wake.

Pass: sensor returns to deep sleep within 600 ms of wake with no gateway present.
