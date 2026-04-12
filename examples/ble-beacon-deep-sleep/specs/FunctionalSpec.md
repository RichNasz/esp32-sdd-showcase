# BLE Beacon + Deep Sleep — Multi-Year Battery Life

## Overview

Duty-cycled BLE beacon example. Wakes from deep sleep, advertises a BLE beacon packet
for 10 seconds with custom manufacturer data (including a boot counter and a fake sensor
reading), then returns to deep sleep for 10 seconds. Button press on GPIO 0 triggers an
immediate wakeup and advertisement. Demonstrates BLE duty-cycling with a 50% duty cycle
optimised for easy demonstration.

## Supported Boards

| Target | Board | LED GPIO | LED Type | Button GPIO | Button Wakeup | Console | Role |
|---|---|---|---|---|---|---|---|
| `esp32` | Adafruit HUZZAH32 | GPIO 13 (active HIGH) | Simple GPIO | GPIO 0 (RTCIO_CH11) | ✓ ext0 | UART (CP2104) | sole |
| `esp32s3` | YEJMKJ ESP32-S3-DevKitC-1-N16R8 | GPIO 48 | WS2812 RGB | — | ✗ timer-only² | USB Serial/JTAG | **primary** |
| `esp32s3` | Seeed XIAO ESP32S3 | GPIO 21 (active LOW) | Simple GPIO | GPIO 0 (RTC_GPIO0) | ✓ ext1 | USB CDC | secondary |
| `esp32c5` | Seeed XIAO ESP32-C5 | GPIO 27 (active LOW) | Simple GPIO | GPIO 9 | ✗ timer-only¹ | USB Serial/JTAG | sole |
| `esp32c6` | Espressif ESP32-C6-DevKitC-1-N8 | GPIO 8 | WS2812 RGB | — | ✗ timer-only¹ | UART (CH343P) | **primary** |
| `esp32c6` | Seeed XIAO ESP32-C6 | GPIO 15 (active LOW) | Simple GPIO | GPIO 9 | ✗ timer-only¹ | USB Serial/JTAG | secondary |

¹ GPIO 9 is the boot button on XIAO C6/C5, but it is not an RTC-capable GPIO on those targets
(C6: only GPIO 0–7 are RTC-capable; C5: only GPIO 0–6). Timer-only wakeup is used.

² YEJMKJ DevKitC boot button GPIO is not RTC-capable on this variant. Timer-only wakeup is used.

## Multi-Manufacturer Same-Chip Handling

Two boards share the `esp32s3` target and two share the `esp32c6` target. The WS2812 board is
**primary** for each target — its configuration is baked into `sdkconfig.defaults.<target>`. The
simple GPIO board is **secondary** and requires a one-time menuconfig step after `idf.py set-target`.

For XIAO ESP32S3 (secondary on esp32s3):
```sh
idf.py set-target esp32s3
idf.py menuconfig
```
In `BLE Beacon Configuration`, set:
- `Use WS2812 RGB LED` → **Disabled**
- `LED GPIO number` → **21**
- `LED active level` → **0** (active LOW)

Also in `Component config → ESP System Settings → Channel for console output` → **USB CDC**.

For XIAO ESP32-C6 (secondary on esp32c6):
```sh
idf.py set-target esp32c6
idf.py menuconfig
```
In `BLE Beacon Configuration`, set:
- `Use WS2812 RGB LED` → **Disabled**
- `LED GPIO number` → **15**
- `LED active level` → **0** (active LOW)

## Requirements

- Wake from deep sleep on a 10-second RTC timer wakeup (all targets).
- On targets where the boot button is an RTC-capable GPIO (ESP32-S3, ESP32), also configure GPIO
  wakeup for on-demand advertisement: ext1 on ESP32-S3 (GPIO 0), ext0 on original ESP32 (GPIO 0).
  On ESP32-C6 and ESP32-C5, timer-only wakeup is used (boot button GPIO 9 is not RTC-capable).
- Advertise a single BLE advertisement burst (10 s advertising window) with:
  - Complete Local Name AD element: `"ESP32-SDD"` — appears by name in BLE scanner apps.
  - Company ID: 0xFFFF (test/example use per Bluetooth SIG)
  - Custom manufacturer data: `[boot_count (2 bytes LE)][sequence (2 bytes LE)][0xDE 0xAD 0xBE 0xEF]`
  - Total ADV PDU must remain ≤ 31 bytes.
- Stop advertising after 10 s advertising window, then return to deep sleep immediately.
- Increment `RTC_DATA_ATTR uint16_t boot_count` and `RTC_DATA_ATTR uint16_t sequence` on every wake.
- Turn the LED **on when BLE advertising is active** (immediately after `ble_gap_adv_start()` succeeds) and **off when advertising stops** (immediately after `ble_gap_adv_stop()` in the advertising-complete path). The LED must remain dark during the BLE initialisation phase and during teardown. For **WS2812 RGB LED boards** (YEJMKJ DevKitC, ESP32-C6-DevKitC-1-N8): use solid blue at 64/255 brightness — blue is the convention for BLE activity. For **simple GPIO boards**: use the board's active level. LED GPIO and polarity are board-specific.
- On every wake cycle, after NimBLE host sync, log the device BLE MAC address in the format
  `BLE MAC: XX:XX:XX:XX:XX:XX` so the user can find the device by MAC in a scanner app.
- Log advertisement payload and wakeup cause over serial on every cycle.
- On boards with USB CDC console (e.g. XIAO ESP32S3), the serial connection drops when
  the device enters deep sleep. Serial output is only available while the LED is on
  (active window). The monitor reconnects automatically on the next wake cycle.

## Hardware Dependencies

- board-specs/adafruit/huzzah32.md — ESP32, LED GPIO 13 active-HIGH, GPIO 0 boot button (RTCIO_CH11)
- board-specs/yejmkj/esp32-s3-devkitc-1-n16r8.md — ESP32-S3, WS2812 RGB LED GPIO 48, USB Serial/JTAG
- board-specs/seeed/xiao-esp32s3.md — ESP32-S3, LED GPIO 21 active-LOW, GPIO 0 boot button (RTC_GPIO0)
- board-specs/seeed/xiao-esp32c5.md — ESP32-C5, LED GPIO 27 active-LOW, GPIO 9 boot button (not RTC-capable)
- board-specs/espressif/esp32-c6-devkitc-1-n8.md — ESP32-C6, WS2812 RGB LED GPIO 8, UART bridge (CH343P)
- board-specs/seeed/xiao-esp32c6.md — ESP32-C6, LED GPIO 15 active-LOW, GPIO 9 boot button (not RTC-capable)

## Power Budget

| State | Current | Duration |
|---|---|---|
| Deep sleep | ~14 µA | ~10 s |
| Active + BLE TX | ~30 mA | ~10 s |
| Target average (20 s cycle) | < 15 mA | — |

## Success Criteria

- BLE advertisement packet is visible to a scanner (e.g. nRF Connect app) on every wake cycle.
- Device appears as **`"ESP32-SDD"`** in scanner app device lists without requiring any MAC filter.
- Manufacturer data in the packet matches expected `boot_count` and `sequence` values.
- Serial log on every cycle includes the BLE MAC address (`BLE MAC: XX:XX:XX:XX:XX:XX`).
- Active window does not exceed 12 s. LED is dark during init and teardown phases.
- On boards with RTC-capable boot button (ESP32-S3, ESP32): button press wakes device and triggers
  an advertisement within 500 ms.
- No BLE stack crashes or watchdog triggers across 20+ consecutive cycles.
