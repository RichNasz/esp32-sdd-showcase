<!-- ================================================
     AGENT-GENERATED — DO NOT EDIT BY HAND
     Generated from specs/ using esp32-sdd-documentation-generator skill
     Date: 2026-03-13 | Agent: Claude Code
     ================================================ -->

# ESP-NOW Low-Power Mesh — Wi-Fi-Free Networking

> **Infrastructure-free sensor network.** Two roles on one firmware: sleep → transmit sensor node and always-on gateway, communicating via ESP-NOW without a Wi-Fi router or cloud.

## Overview

A single firmware binary supports two compile-time roles selected by a Kconfig option:

- **Sensor node**: wakes from deep sleep every 20 seconds, sends one ESP-NOW packet (node ID,
  message count, fake temperature) to the gateway MAC address, blinks the LED on success, and
  returns to sleep. Active window ≤ 300 ms.
- **Gateway node**: always on, registers an ESP-NOW receive callback, logs each received packet
  over serial, and blinks the LED once per packet.

Both roles use Wi-Fi in STA mode (no AP association) — required by ESP-NOW on ESP32-S3.

## Board

| Item | Detail |
|---|---|
| Board | Seeed XIAO ESP32S3 (both roles) |
| Target chip | ESP32-S3 |
| ESP-IDF | v5.5.x |
| LED | GPIO 21, active LOW |
| Hardware needed | 2× XIAO ESP32S3 for full end-to-end test |

## Build & Flash

```sh
cd examples/esp-now-low-power-mesh
idf.py set-target esp32s3

# --- Flash the GATEWAY board first ---
# In menuconfig: set NODE_ROLE = GATEWAY
idf.py menuconfig
idf.py build flash monitor
# Note the gateway MAC from the serial log.

# --- Flash the SENSOR board ---
# In menuconfig: set NODE_ROLE = SENSOR, set GATEWAY_MAC, NODE_ID = 1
idf.py menuconfig
idf.py build flash monitor
```

## Expected Serial Output

**Gateway:**
```
I (312) gateway: ESP-NOW gateway ready — listening...
I (20415) gateway: RX from AA:BB:CC:DD:EE:01 | node=1 msg=3 temp=0.30
I (40417) gateway: RX from AA:BB:CC:DD:EE:01 | node=1 msg=4 temp=0.40
```

**Sensor node:**
```
I (312) sensor: Wake #3 | cause: timer
I (315) sensor: Sending to gateway AA:BB:CC:11:22:33 ...
I (412) sensor: ESP-NOW send OK
I (413) sensor: LED blink
I (514) sensor: Entering deep sleep for 20 s...
```

## Key Concepts

- ESP-NOW requires `WIFI_MODE_STA` even without AP association — omitting Wi-Fi init causes
  `esp_now_init()` to fail
- Fixed Wi-Fi channel — all nodes must use the same channel; mismatch is the most common cause
  of delivery failures
- Compact payload struct `espnow_payload_t` with a compile-time
  `static_assert(sizeof(espnow_payload_t) <= 250)` enforcing the ESP-NOW maximum at build time
- Binary semaphore synchronises the ESP-NOW send callback with `app_main` (no busy-waiting)
- Sensor node deinitialises ESP-NOW and stops Wi-Fi before sleeping — leaving the radio active
  defeats the power budget
- Gateway MAC is a Kconfig string parsed into a byte array at runtime
- Peers must be registered via `esp_now_add_peer()` before calling `esp_now_send()`

## Power Budget (sensor node)

| State | Current | Duration |
|---|---|---|
| Deep sleep | ~14 µA | ~20 s |
| Active + ESP-NOW TX | ~50 mA | ~200 ms |
| Target average | < 500 µA | — |

## Testing

Testing philosophy: automated first, manual only when hardware observation is unavoidable.

### Automated Tests

1. **T-A1 — Zero-Warning Build (Sensor Role)**

   ```sh
   cd examples/esp-now-low-power-mesh
   idf.py set-target esp32s3 && idf.py build
   ```

   Pass: exit code 0, zero compiler warnings (default sensor role in sdkconfig.defaults).

2. **T-A2 — Zero-Warning Build (Gateway Role)**

   ```sh
   echo "CONFIG_NODE_ROLE_GATEWAY=y" >> sdkconfig.defaults
   idf.py fullclean && idf.py build
   git checkout sdkconfig.defaults   # restore
   ```

   Pass: exit code 0, zero compiler warnings for gateway role.

3. **T-A3 — Payload Size Compile-Time Assert**

   Pass: build succeeds, confirming `sizeof(espnow_payload_t) <= 250` (ESP-NOW max payload).
   The `static_assert` fires at compile time if the struct grows too large — this test requires
   no additional tooling beyond `idf.py build`.

### Manual Tests — hardware required

**T-M1 — Sensor-to-Gateway Packet Delivery**

Why manual: real ESP-NOW packet exchange over the air requires two physical boards.

Hardware: 2× XIAO ESP32S3.

Setup:
1. Flash board A as gateway — set `CONFIG_NODE_ROLE_GATEWAY=y`, build and flash. Note its MAC from serial log.
2. Flash board B as sensor — set `CONFIG_GATEWAY_MAC` to board A's MAC, `CONFIG_NODE_ID=1`, build and flash.

Steps:
1. Open gateway serial monitor.
2. Watch for 5 consecutive receive log lines from sensor node 1.
3. Confirm `msg_count` increments by 1 on each packet (no drops).
4. Confirm `fake_temp` is a plausible float (0.1–26.0 for node_id=1).

Pass: 5/5 packets received; msg_count monotonically increases.

---

**T-M2 — Sensor Active Window ≤ 300 ms**

Why manual: precise sleep/wake timing requires serial timestamp observation on real hardware.

1. Monitor sensor node serial output.
2. Timestamp from wake log to "sleeping" log must be ≤ 300 ms.

Pass: active window ≤ 300 ms per serial timestamps.

---

**T-M3 — Sensor Sleeps Even if Gateway Unreachable**

Why manual: requires physical absence of the gateway board to test the timeout path.

1. Flash sensor node without a gateway present.
2. Confirm sensor wakes, attempts send, logs a failure, then returns to deep sleep within 500 ms.
3. Confirm no blocking hang.

Pass: sensor returns to deep sleep within 500 ms of wake even with no gateway.

## Spec Files

- [specs/FunctionalSpec.md](specs/FunctionalSpec.md)
- [specs/CodingSpec.md](specs/CodingSpec.md)
- [specs/TestSpec.md](specs/TestSpec.md)
