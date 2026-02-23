# ESP-NOW Low-Power Mesh — Wi-Fi-Free Networking

## Overview

Infrastructure-free sensor network example. Two roles run on the same firmware,
selected by a Kconfig option: **sensor node** (sleeps 20 seconds between transmissions,
wakes to send one ESP-NOW packet, then sleeps again) and **gateway node** (stays awake,
receives packets from all sensor nodes, logs data over serial). Demonstrates low-power
peer-to-peer ESP-NOW communication without a Wi-Fi router or cloud.

## Supported Boards

- Seeed XIAO ESP32S3 (default, for both roles)

## Requirements

### Common (both roles)

- ESP-NOW initialised in Wi-Fi STA mode (no AP association required).
- Device role selected via `CONFIG_NODE_ROLE` Kconfig option: `SENSOR` or `GATEWAY`.
- Shared struct for packet payload:
  ```c
  typedef struct {
      uint8_t  node_id;       // sender identifier (1–255)
      uint16_t msg_count;     // RTC_DATA_ATTR counter
      float    fake_temp;     // placeholder: node_id * 0.1f + (msg_count % 10)
  } espnow_payload_t;
  ```

### Sensor Node Role

- Wake from deep sleep every 20 seconds using RTC timer.
- On wake: increment `RTC_DATA_ATTR uint16_t msg_count`.
- Send one ESP-NOW unicast packet to the gateway MAC address (`CONFIG_GATEWAY_MAC`).
- Wait up to 200 ms for send callback confirmation (`esp_now_send` callback).
- Blink LED (GPIO 21) once on successful send; fast-blink on failure.
- Return to deep sleep after send (or timeout).

### Gateway Node Role

- Never sleep; run in a continuous receive loop.
- Register an `esp_now_recv_cb_t` callback.
- On each received packet: log `node_id`, `msg_count`, and `fake_temp` over serial.
- Blink LED once per received packet.
- Accept packets from any MAC address (no pre-registered peer required in promiscuous mode).

## Hardware Dependencies

- Board-spec: board-specs/seeed/xiao-esp32s3.md (ESP32-S3, LED GPIO 21 active LOW)
- No external components required.
- Two XIAO ESP32S3 boards needed for full end-to-end test (one per role).

## Power Budget (sensor node)

| State | Current | Duration |
|---|---|---|
| Deep sleep | ~14 µA | ~20 s |
| Active + ESP-NOW TX | ~50 mA | ~200 ms |
| Target average | < 500 µA | — |

## Success Criteria

- Gateway receives packets from sensor node on every wake cycle.
- `msg_count` increments monotonically in received packets.
- Active window on sensor node ≤ 300 ms per cycle.
- Gateway logs all packets without drops in a stable RF environment.
- Sensor node returns to deep sleep even if gateway is unreachable (no blocking wait).
