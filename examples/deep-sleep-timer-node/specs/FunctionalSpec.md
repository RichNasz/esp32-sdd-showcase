# Deep Sleep Timer Node — RTC Wakeup + Persistence

## Overview

Baseline deep-sleep example. Demonstrates the core wake-measure-persist-sleep loop
that every battery-powered ESP32 project depends on. An RTC timer wakeup fires every
15 seconds; the firmware increments a boot counter stored in RTC memory, logs the
count and wakeup cause over serial, blinks the onboard LED once as visible confirmation,
then returns to deep sleep immediately.

## Supported Boards

- Seeed XIAO ESP32S3 (default)

## Requirements

- Wake from deep sleep on a 15-second RTC timer wakeup.
- On every wake: increment a `RTC_DATA_ATTR uint32_t boot_count` (survives deep sleep without NVS overhead).
- Detect first boot (`ESP_SLEEP_WAKEUP_UNDEFINED`) vs timer wakeup (`ESP_SLEEP_WAKEUP_TIMER`) and log accordingly.
- Log boot count and wakeup cause over serial on every wake.
- Blink the onboard LED (GPIO 21, active LOW) once per wake as a visible heartbeat.
- Return to deep sleep after ~200 ms active window.
- Zero external components required.

## Hardware Dependencies

- Board-spec: board-specs/seeed/xiao-esp32s3.md (ESP32-S3, LED GPIO 21 active LOW)

## Power Budget

| State | Current | Duration |
|---|---|---|
| Deep sleep | ~14 µA | 15 s |
| Active (CPU + USB-CDC) | ~68 mA | ~200 ms |
| Target average | < 1 mA | — |

## Success Criteria

- Boot counter increments by 1 on every wakeup cycle with no skips.
- Serial output shows correct wakeup cause and count on every wake.
- LED blinks exactly once per wake cycle; off during sleep.
- No stack overflows or watchdog triggers across 20+ consecutive cycles.
