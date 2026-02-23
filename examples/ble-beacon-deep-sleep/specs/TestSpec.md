# Testing Specification — BLE Beacon + Deep Sleep

## Testing Philosophy

Automated testing using native ESP-IDF tools is strongly preferred.
Additional tools are only allowed with explicit justification and documented benefits.
Manual testing is used only when no practical automated alternative exists.

---

## Automated Tests

### T-A1 — Zero-Warning Build

**Tool**: `idf.py` (native)

```sh
cd examples/ble-beacon-deep-sleep
idf.py set-target esp32s3 && idf.py build
```

Pass: exit code 0, zero compiler warnings.

---

### T-A2 — Binary Size Check

**Tool**: `idf.py` (native)

```sh
idf.py size | grep "Total binary size"
```

Pass: < 1.5 MB (NimBLE stack included).

---

### T-A3 — NimBLE Config Check

**Tool**: `idf.py` (native)

```sh
grep "CONFIG_BT_NIMBLE_ENABLED=y" examples/ble-beacon-deep-sleep/sdkconfig.defaults
```

Pass: line present.

---

## Manual Tests

*BLE advertisement scanning and real hardware deep-sleep/wake cycles require physical hardware and a BLE scanner.*

### T-M1 — BLE Advertisement Visible to Scanner

**Why manual**: Verifying OTA BLE advertisement content requires a second device acting as a scanner — not reproducible in simulation.

Prerequisites: smartphone with nRF Connect (iOS/Android) or equivalent BLE scanner.

1. Flash to XIAO ESP32S3: `idf.py set-target esp32s3 && idf.py build flash`
2. Open nRF Connect → Scanner tab → start scanning.
3. On each device wake (every 20 seconds), confirm a 100 ms advertisement burst is visible.
4. Inspect raw manufacturer data — confirm company ID `0xFFFF` and 4-byte magic `DE AD BE EF` are present.
5. Confirm `boot_count` field increments by 1 on each wake.

Pass: advertisement appears on every cycle; manufacturer data matches spec.

---

### T-M2 — Active Window ≤ 200 ms

**Why manual**: Precise active window timing requires a logic analyser or oscilloscope, or observation of LED blink duration.

1. Observe LED blink during advertising window.
2. LED blink must be brief (≈100 ms); device must return to sleep quickly.
3. Serial monitor must show adv-start then adv-done within the same second timestamp.

Pass: LED blink ≈ 100 ms; total active window ≤ 200 ms per serial timestamps.

---

### T-M3 — Button Wakeup Triggers Immediate Advertisement

**Why manual**: Physical ext0 GPIO wakeup requires real hardware.

1. While device is in deep sleep, press and release the BOOT button (GPIO 0).
2. Confirm advertisement burst and LED blink occur within 500 ms of button press.
3. Confirm serial log shows `EXT0` wakeup cause.

Pass: advertisement triggered within 500 ms of button press.
