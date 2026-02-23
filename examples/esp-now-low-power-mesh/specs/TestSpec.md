# Testing Specification — ESP-NOW Low-Power Mesh

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

Pass: exit code 0, zero compiler warnings (default sensor role in sdkconfig.defaults).

---

### T-A2 — Zero-Warning Build (Gateway Role)

**Tool**: `idf.py` (native)

```sh
echo "CONFIG_NODE_ROLE_GATEWAY=y" >> sdkconfig.defaults
idf.py fullclean && idf.py build
git checkout sdkconfig.defaults  # restore
```

Pass: exit code 0, zero compiler warnings for gateway role.

---

### T-A3 — Payload Size Compile-Time Assert

**Tool**: `idf.py` (native — static assert fires at build time)

Pass: build succeeds, confirming `sizeof(espnow_payload_t) <= 250` (ESP-NOW max payload).

---

## Manual Tests

*ESP-NOW packet exchange requires two physical boards. Inter-device RF communication cannot be simulated without a two-device Wokwi CI setup.*

### T-M1 — Sensor-to-Gateway Packet Delivery

**Why manual**: Real ESP-NOW packet exchange over the air requires two XIAO ESP32S3 boards.

Hardware: 2× XIAO ESP32S3.

Setup:
1. Flash board A as gateway: set `CONFIG_NODE_ROLE_GATEWAY=y`, build and flash. Note its MAC address from serial log.
2. Flash board B as sensor: set `CONFIG_GATEWAY_MAC` to board A's MAC, `CONFIG_NODE_ID=1`, build and flash.

Steps:
1. Open gateway serial monitor.
2. Watch for 5 consecutive receive log lines from sensor node 1.
3. Confirm `msg_count` increments by 1 on each packet (no drops).
4. Confirm `fake_temp` is a plausible float (0.1–26.0 range for node_id=1).

Pass: 5/5 packets received; msg_count monotonically increases.

---

### T-M2 — Sensor Active Window ≤ 300 ms

**Why manual**: Precise sleep/wake timing requires serial timestamp observation on real hardware.

1. Monitor sensor node serial output.
2. Timestamp from wake log to "sleeping" log must be ≤ 300 ms.

Pass: active window ≤ 300 ms per serial timestamps.

---

### T-M3 — Sensor Sleeps Even if Gateway Unreachable

**Why manual**: Requires physical absence of the gateway board to test timeout path.

1. Flash sensor node without a gateway present.
2. Confirm sensor wakes, attempts send, logs "FAIL", then returns to deep sleep within 500 ms.
3. Confirm no blocking hang.

Pass: sensor returns to deep sleep within 500 ms of wake even with no gateway.
