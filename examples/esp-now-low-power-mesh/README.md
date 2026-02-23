<!-- ================================================
     AGENT-GENERATED — DO NOT EDIT BY HAND
     Generated from specs/ using esp32-sdd-documentation-generator skill
     Date: 2026-02-22 | Agent: Claude Code
     ================================================ -->

# ESP-NOW Low-Power Mesh — Wi-Fi-Free Networking

> **Infrastructure-free mesh.** Multi-node sensor network using ESP-NOW — no router, no cloud, sub-10 ms latency, and deep sleep between transmissions on sensor nodes.

## What It Demonstrates

- ESP-NOW peer-to-peer and broadcast messaging
- Two-role topology: sensor nodes → gateway node → serial / UART output
- Deep sleep between transmissions on sensor nodes; gateway always-on
- Automatic peer discovery using broadcast MAC + MAC registration handshake

## Target Hardware

| Item | Detail |
|---|---|
| Minimum nodes | 2 ESP32 boards (1 gateway + 1 sensor) |
| Recommended | 3–5 nodes for a meaningful mesh demo |
| Range | ~200 m open air with PCB antenna; ~700 m with external antenna |
| All boards | Must use the same Wi-Fi channel (configured at compile time) |

## Build & Flash

```sh
# Gateway node
cd examples/esp-now-low-power-mesh
idf.py -DNODE_ROLE=gateway build
idf.py -p /dev/ttyUSB0 flash monitor

# Each sensor node (separate terminal per node)
idf.py -DNODE_ROLE=sensor build
idf.py -p /dev/ttyUSB1 flash
```

## Expected Output (Gateway)

```
I (512) mesh_gw: Channel 6 — listening for peer registrations
I (820) mesh_gw: Registered peer AA:BB:CC:DD:EE:01
I (3240) mesh_gw: [AA:BB:CC:01] T=24.1°C  RSSI=-62 dBm  seq=14  bat=3.72 V
I (9810) mesh_gw: [AA:BB:CC:02] T=22.8°C  RSSI=-71 dBm  seq=9   bat=3.68 V
```

## Key Concepts

- `esp_now_init()` + `esp_now_register_recv_cb()` + `esp_now_register_send_cb()`
- Discovery: broadcast `esp_now_send(broadcast_mac, ...)` → gateway replies with unicast ACK
- `esp_wifi_set_channel()` — all nodes must share the same primary channel
- `RTC_DATA_ATTR static uint32_t seq` — sequence counter survives deep sleep on sensor nodes

## Power Budget (Sensor Node, 30 s interval)

| State | Current | Duration |
|---|---|---|
| Wake + ESP-NOW TX | ~100 mA peak / ~60 mA avg | ~80 ms |
| Deep sleep | ~10 µA | 29.92 s |
| **Average** | **~170 µA** | — |

## Spec Files

- [FunctionalSpec.md](FunctionalSpec.md)
- [CodingSpec.md](CodingSpec.md)
