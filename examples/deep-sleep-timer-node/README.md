<!-- ================================================
     AGENT-GENERATED — DO NOT EDIT BY HAND
     Generated from specs/ using esp32-sdd-documentation-generator skill
     Date: 2026-02-22 | Agent: Claude Code
     ================================================ -->

# Deep Sleep Timer Node — RTC Wakeup + NVS Persistence

> **Power foundation.** Demonstrates the core deep-sleep / wake / persist loop that underpins every battery-powered ESP32 application.

## What It Demonstrates

- `esp_deep_sleep_start()` with RTC timer wakeup source
- NVS (Non-Volatile Storage) for persisting counters and state across resets
- Boot-reason inspection via `esp_sleep_get_wakeup_cause()`
- RTC GPIO hold (`rtc_gpio_hold_en()`) to maintain output state during sleep

## Target Hardware

| Item | Detail |
|---|---|
| Board | Any ESP32 devkit (ESP32-DevKitC recommended) |
| Sleep current | ~10 µA typical |
| Wake interval | 10 s (configurable via `idf.py menuconfig`) |

## Build & Flash

```sh
cd examples/deep-sleep-timer-node
idf.py set-target esp32
idf.py build flash monitor
```

## Expected Output

```
I (312) sleep_node: Boot #7 | wakeup: timer | active for 218 ms
I (315) sleep_node: Counter persisted to NVS (key: "boot_count")
I (317) sleep_node: Entering deep sleep for 10 s...
```

## Key Concepts

- `esp_sleep_enable_timer_wakeup(10 * 1000000ULL)`
- `nvs_flash_init()` → `nvs_open()` → `nvs_get_i32()` / `nvs_set_i32()` → `nvs_commit()`
- `esp_sleep_get_wakeup_cause()` → `ESP_SLEEP_WAKEUP_TIMER`
- `RTC_DATA_ATTR` for lightweight variables that survive deep sleep without NVS

## Power Budget

| State | Current | Duration |
|---|---|---|
| Active (CPU @ 240 MHz) | ~80 mA | ~220 ms |
| Deep sleep | ~10 µA | 9.78 s |
| **Average** | **~1.96 mA** | — |

## Spec Files

- [FunctionalSpec.md](FunctionalSpec.md)
- [CodingSpec.md](CodingSpec.md)
