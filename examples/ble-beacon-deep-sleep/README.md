<!-- ================================================
     AGENT-GENERATED — DO NOT EDIT BY HAND
     Generated from specs/ using esp32-sdd-documentation-generator skill
     Date: 2026-03-13 | Agent: Claude Code
     ================================================ -->

# BLE Beacon + Deep Sleep — Multi-Year Battery Life

> **Ultra-low-power BLE.** Duty-cycled beacon that wakes, advertises for 10 seconds, and returns to deep sleep — all on a single firmware image that runs on four boards via `idf.py set-target`.

## Overview

On each wake cycle: increment RTC counters, broadcast a non-connectable BLE advertisement
packet (name `"ESP32-SDD"`, boot count, sequence, 4-byte magic) for 10 seconds using the
NimBLE stack, tear down the BLE stack completely, turn off the LED, and enter deep sleep for
10 seconds. The LED is lit during the advertising window. Pressing the boot button triggers an
immediate wakeup and advertisement on boards with an RTC-capable boot button (ESP32-S3, ESP32).

The BLE MAC address is logged on every wake so you can find the device by name in a scanner app
(e.g. nRF Connect) — search for `"ESP32-SDD"`.

> **Serial monitor on USB CDC boards**: on boards using USB CDC (e.g. XIAO ESP32S3), the USB
> connection drops when the device enters deep sleep. Serial output is only available while the
> LED is on. The monitor reconnects automatically when the device wakes on the next cycle.

## Supported Boards

| `idf.py set-target` | Board | LED GPIO | Active | Button GPIO | Button Wakeup |
|---|---|---|---|---|---|
| `esp32s3` (default) | Seeed XIAO ESP32S3 | GPIO 21 | LOW | GPIO 0 | ext1 |
| `esp32c6` | Seeed XIAO ESP32-C6 | GPIO 15 | LOW | GPIO 9 | timer-only¹ |
| `esp32c5` | Seeed XIAO ESP32-C5 | GPIO 27 | LOW | GPIO 9 | timer-only¹ |
| `esp32` | Adafruit HUZZAH32 | GPIO 13 | HIGH | GPIO 0 | ext0 |

¹ GPIO 9 (boot button on XIAO C6/C5) is not an RTC-capable GPIO on those targets. Timer-only
wakeup is used instead.

## Build & Flash

```sh
cd examples/ble-beacon-deep-sleep

# Select your target board:
idf.py set-target esp32s3   # or esp32c6, esp32c5, esp32

idf.py build flash monitor
```

> **Board switching**: delete `sdkconfig` and `sdkconfig.old` before switching targets to
> ensure the new per-target `sdkconfig.defaults.<target>` takes full effect.

## Advertising Payload

| Field | Value |
|---|---|
| Complete Local Name | `"ESP32-SDD"` |
| Company ID | `0xFFFF` (test/example use per Bluetooth SIG) |
| Manufacturer data | `[boot_count 2B LE][sequence 2B LE][0xDE 0xAD 0xBE 0xEF]` |
| ADV PDU total | 25 bytes (limit: 31 bytes) |

## Expected Serial Output

```
I (213) ble_beacon: Wakeup: TIMER
I (213) ble_beacon: Cycle: boot_count=2 seq=2
I (283) ble_beacon: BLE MAC: B8:F8:62:F8:E0:46
I (303) ble_beacon: Advertising: name=ESP32-SDD boot_count=2 seq=2
I (10303) ble_beacon: BLE stack torn down
I (10313) ble_beacon: Entering deep sleep (10 s). Active window complete.
```

## Key Concepts

- NimBLE stack (not Bluedroid) — significantly lighter initialisation and teardown;
  critical for duty-cycled applications
- `nimble_port_init()` internally calls `esp_bt_controller_init/enable`; do **not** call
  those explicitly or it will double-init and crash
- `CONFIG_BT_NIMBLE_SECURITY_ENABLE=n` required for broadcaster-only builds on ESP-IDF 5.5.x
  (workaround for a `ble_hs.c` / `ble_sm.c` linker guard mismatch)
- Complete Local Name AD element (`"ESP32-SDD"`) makes the beacon findable by name in
  scanner apps without MAC filtering
- `RTC_DATA_ATTR uint16_t boot_count` and `sequence` persist across deep sleep cycles
- `gpio_hold_dis()` before driving LED + `gpio_hold_en()` after LED off, before sleep
- `nvs_flash_init()` must be called before any BLE API (BLE stores PHY calibration in NVS)
- ext1 wakeup on ESP32-S3 (GPIO 0); ext0 on original ESP32 (GPIO 0); timer-only on C6/C5

## Power Budget

| State | Current | Duration |
|---|---|---|
| Deep sleep | ~14 µA | ~10 s |
| Active + BLE TX | ~30 mA | ~10 s |
| Target average (20 s cycle) | < 15 mA | — |

## Testing

Testing philosophy: automated first, manual only when hardware observation is unavoidable.

### Automated Tests

1. **T-A1 — Zero-Warning Build**

   ```sh
   cd examples/ble-beacon-deep-sleep
   idf.py set-target esp32s3 && idf.py build
   ```

   Pass: exit code 0, zero compiler warnings.

2. **T-A2 — Binary Size Check**

   ```sh
   idf.py size | grep "Total binary size"
   ```

   Pass: < 1.5 MB (NimBLE stack included).

3. **T-A3 — NimBLE Config Check**

   ```sh
   grep "CONFIG_BT_NIMBLE_ENABLED=y" examples/ble-beacon-deep-sleep/sdkconfig.defaults
   ```

   Pass: line present.

### Manual Tests — hardware required

**T-M1 — BLE Advertisement Visible to Scanner**

Why manual: verifying BLE advertisement content requires a second device acting as a
scanner — not reproducible in simulation.

Prerequisites: smartphone with nRF Connect (iOS/Android) or equivalent BLE scanner.

1. Flash to XIAO ESP32S3: `idf.py set-target esp32s3 && idf.py build flash monitor`
2. Note the `BLE MAC: XX:XX:XX:XX:XX:XX` line from serial (available while LED is on).
3. Open nRF Connect → Scanner tab → start scanning.
4. On each device wake (~every 20 seconds), locate **`"ESP32-SDD"`** in the device list.
5. Inspect raw manufacturer data — confirm company ID `0xFFFF` and magic `DE AD BE EF`.
6. Confirm `boot_count` field increments by 1 on each wake.

Pass: advertisement appears on every cycle; manufacturer data matches spec.

---

**T-M2 — Active Window ≤ 12 s**

Why manual: precise active window timing requires serial timestamp observation.

1. Observe serial timestamps from "Advertising:" to "Entering deep sleep".
2. Total active window must be ≤ 12 seconds.

Pass: total active window ≤ 12 s per serial timestamps.

---

**T-M3 — Button Wakeup Triggers Immediate Advertisement**

Why manual: physical ext1/ext0 GPIO wakeup requires real hardware. Only applicable on
ESP32-S3 and HUZZAH32 — skip on C6/C5.

1. While device is in deep sleep (LED off), press and release the BOOT button (GPIO 0).
2. Confirm advertisement burst and LED turn on within 500 ms of button press.
3. Confirm serial log shows `EXT1` (ESP32-S3) or `EXT0` (HUZZAH32) as the wakeup cause.

Pass: advertisement triggered within 500 ms of button press.

## Spec Files

- [specs/FunctionalSpec.md](specs/FunctionalSpec.md)
- [specs/CodingSpec.md](specs/CodingSpec.md)
- [specs/TestSpec.md](specs/TestSpec.md)
