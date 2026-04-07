# ESP-NOW Low-Power Mesh with WiFi RSSI Sensing

## Overview

Infrastructure-free multi-hop sensor network. Three roles run on the same firmware,
selected by a Kconfig option: **sensor node** (sleeps 60 seconds between transmissions,
wakes to scan nearby WiFi access points for signal strength, packages real RSSI data
into an ESP-NOW packet, routes the packet toward the gateway through one or more hops,
then sleeps again); **relay node** (always-on, receives sensor packets and forwards them
toward the gateway, adding a hop to the path, deduplicating retransmits); and
**gateway node** (always-on, broadcasts a beacon so nodes can discover it without a
hardcoded MAC address, receives and logs all packets, records hop count and per-SSID
RSSI values).

This example demonstrates:

- True multi-hop ESP-NOW mesh routing (not hub-and-spoke)
- Dynamic gateway discovery via beacon — no compile-time MAC hardcoding
- Real sensor data: WiFi RSSI from up to three configured SSIDs
- Deep sleep duty cycling with RTC memory used for both counters and topology state
- Per-packet hop counting as evidence of relay path usage

## Supported Boards

- Seeed XIAO ESP32S3 (default, all roles use the same board)

## Roles

### Role 1 — SENSOR

- Wake from deep sleep every 60 seconds using the RTC timer.
- On wake: increment `RTC_DATA_ATTR uint16_t msg_count`.
- If `RTC_DATA_ATTR` gateway MAC cache is empty (all zeros), listen on the ESP-NOW
  channel for a gateway beacon for up to 2 seconds. Cache the sender's MAC address.
  If no beacon arrives within 2 seconds, log a warning and return to deep sleep.
- Run an active WiFi scan targeting up to three SSIDs configured via Kconfig
  (`CONFIG_TARGET_SSID_1`, `CONFIG_TARGET_SSID_2`, `CONFIG_TARGET_SSID_3`).
  Record RSSI (dBm, int8_t) for each found SSID. Use `INT8_MIN` (–128) for SSIDs
  not found during the scan window.
- Deinitialize the WiFi scan and reinitialize ESP-NOW on the fixed channel.
- Send one ESP-NOW unicast packet to the cached gateway MAC. Set `hop_count = 0`.
- Wait up to 200 ms for the send callback. On success: blink LED once. On failure:
  fast-blink LED, invalidate the gateway MAC cache in RTC memory, and log a warning.
- Deinitialize ESP-NOW and stop WiFi cleanly before entering deep sleep.

### Role 2 — RELAY

- Never deep sleep. Run continuously in receive mode on the fixed ESP-NOW channel.
- On receiving any `espnow_payload_t` packet:
  - Check a static dedup table (node_id + msg_count pair). If already seen, drop silently.
  - Record the pair in the dedup table.
  - Increment `hop_count` by 1.
  - Forward the packet via ESP-NOW unicast to the cached gateway MAC.
  - Blink LED once.
- Listen for gateway beacon on startup and cache the gateway MAC (same mechanism as sensor).
- Log forwarded packets over serial (node_id, msg_count, hop_count, RSSI values).

### Role 3 — GATEWAY

- Never sleep. Run continuously in receive mode.
- On startup: broadcast a beacon packet every 5 seconds to the ESP-NOW broadcast
  address (`FF:FF:FF:FF:FF:FF`). The beacon carries only a type byte and sequence
  number; the gateway MAC is implicit from the sender address seen by the receiver.
- Register an `esp_now_recv_cb_t` callback. On each received `espnow_payload_t`:
  log node_id, msg_count, hop_count, and all three RSSI values over serial.
  Blink LED once per received packet.
- Accept packets from any MAC address. Do not require pre-registered peers in
  the receive path.
- Log beacon sequence number over serial on each broadcast for diagnostics.

## Shared Payload Struct

```c
typedef struct __attribute__((packed)) {
    uint8_t  node_id;    /* sender identifier, 1–254 (Kconfig)     */
    uint16_t msg_count;  /* monotonic counter, RTC_DATA_ATTR        */
    uint8_t  hop_count;  /* 0 = direct to gateway, 1+ = relayed    */
    int8_t   rssi[3];    /* RSSI per configured SSID; INT8_MIN=none */
    uint8_t  _pad;       /* reserved, set to 0                      */
} espnow_payload_t;      /* 8 bytes — well under 250-byte ESP-NOW limit */
```

## Hardware Dependencies

- Board-spec: `board-specs/seeed/xiao-esp32s3.md` (ESP32-S3, LED GPIO 21 active LOW)
- No external components required.
- Minimum deployment for full mesh demonstration: 3 × XIAO ESP32S3 boards
  (1 gateway, 1 relay, 1 sensor).
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
