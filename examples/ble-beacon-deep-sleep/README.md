<!-- ================================================
     AGENT-GENERATED — DO NOT EDIT BY HAND
     Generated from specs/ using esp32-sdd-documentation-generator skill
     Date: 2026-02-22 | Agent: Claude Code
     ================================================ -->

# BLE Beacon + Deep Sleep — Multi-Year Battery Life

> **Extreme low power.** Achieves multi-year coin-cell operation by combining Bluetooth LE advertising with aggressive deep sleep between beacon cycles.

## What It Demonstrates

- ESP32 BLE advertising in iBeacon and Eddystone-URL formats
- Duty-cycled BLE: advertise for 100 ms, deep sleep for 10 s
- Boot count and sequence number persisted in `RTC_DATA_ATTR` (no NVS overhead)
- Power budget analysis targeting < 5 µA average on a CR2032 cell

## Target Hardware

| Item | Detail |
|---|---|
| Board | Any ESP32 with Bluetooth |
| Target battery | CR2032 (225 mAh) or LiPo |
| Target lifetime | 3+ years at 10 s beacon interval |
| Measured average current | ~4.2 µA |

## Build & Flash

```sh
cd examples/ble-beacon-deep-sleep
idf.py set-target esp32
idf.py build flash monitor
```

## Expected Output

```
I (310) ble_beacon: Boot #142 | advertising iBeacon for 100 ms
I (415) ble_beacon: Advertising complete — stopping BT stack
I (430) ble_beacon: Sleeping 10 s → projected avg: 4.2 µA
```

## Key Concepts

- `esp_ble_gap_start_advertising()` with a timed `esp_ble_gap_stop_advertising()` callback
- `esp_bt_controller_disable()` + `esp_bt_controller_deinit()` before sleep for full BT power-off
- `RTC_DATA_ATTR static uint32_t boot_count` — survives deep sleep, resets on power-on
- Power formula: `(peak_mA × active_s + sleep_µA × sleep_s) / period_s = avg_µA`

## Power Budget

| State | Current | Duration |
|---|---|---|
| BLE advertising active | ~28 mA avg | 100 ms |
| Deep sleep | ~10 µA | 9.9 s |
| **Average** | **~4.2 µA** | — |

At 225 mAh (CR2032) ÷ 4.2 µA ≈ **6.1 years** theoretical lifetime.

## Spec Files

- [FunctionalSpec.md](FunctionalSpec.md)
- [CodingSpec.md](CodingSpec.md)
