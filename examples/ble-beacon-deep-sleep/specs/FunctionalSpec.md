# BLE Beacon + Deep Sleep — Multi-Year Battery Life

## Overview

Extreme low-power example. Wakes from deep sleep every 20 seconds, advertises a BLE
beacon packet with custom manufacturer data (including a boot counter and a fake sensor
reading), then returns to deep sleep. Button press on GPIO 0 triggers an immediate
wakeup and advertisement. Demonstrates how to achieve < 5 µA average current on a
CR2032 cell by duty-cycling BLE.

## Supported Boards

- Seeed XIAO ESP32S3 (default)

## Requirements

- Wake from deep sleep on a 20-second RTC timer wakeup.
- Advertise a single BLE advertisement burst (100 ms advertising window) with:
  - Company ID: 0xFFFF (test/example use per Bluetooth SIG)
  - Custom manufacturer data: `[boot_count (2 bytes LE)][sequence (2 bytes LE)][0xDE 0xAD 0xBE 0xEF]`
- Stop advertising after 100 ms advertising window, then return to deep sleep immediately.
- GPIO 0 (boot button, active LOW) configured as ext0 wakeup for immediate on-demand advertisement.
- Increment `RTC_DATA_ATTR uint16_t boot_count` and `RTC_DATA_ATTR uint16_t sequence` on every wake.
- Blink LED (GPIO 21, active LOW) once during the advertising window.
- Log advertisement payload and wakeup cause over serial on every cycle.

## Hardware Dependencies

- Board-spec: board-specs/seeed/xiao-esp32s3.md (ESP32-S3, LED GPIO 21 active LOW, GPIO 0 boot button)

## Power Budget

| State | Current | Duration |
|---|---|---|
| Deep sleep | ~14 µA | ~19.9 s |
| Active + BLE TX | ~30 mA | ~100 ms |
| Target average (20 s cycle) | < 165 µA | — |

## Success Criteria

- BLE advertisement packet is visible to a scanner (e.g. nRF Connect app) on every wake cycle.
- Manufacturer data in the packet matches expected `boot_count` and `sequence` values.
- Active window does not exceed 200 ms (LED on, advertise, stop, sleep).
- Button press wakes device and triggers an advertisement within 500 ms.
- No BLE stack crashes or watchdog triggers across 20+ consecutive cycles.
