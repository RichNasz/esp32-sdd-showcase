# Capacitive Touch Wakeup — Touch-Driven Interfaces

## Overview

Human-interface deep-sleep example. The device sleeps until a finger touches a touch
pad, wakes instantly, blinks the LED three times as visual confirmation, logs the event
and touch reading over serial, then returns to sleep. A backup 20-second timer wakeup
prevents the device from sleeping indefinitely if the touch pad is never triggered.
Demonstrates the ESP32-S3 capacitive touch sensor peripheral as a zero-power-draw
wakeup source.

## Supported Boards

- Seeed XIAO ESP32S3 (default)

## Requirements

- Configure touch pad T1 (GPIO 1 / D0) as the primary wakeup source.
- Calibrate the touch pad threshold automatically on first boot: sample 50 readings
  in idle state, compute mean, set threshold to mean × 0.7 (30% drop triggers detection).
- Enable deep sleep touch wakeup: `esp_sleep_enable_touchpad_wakeup()`.
- Enable backup RTC timer wakeup: `esp_sleep_enable_timer_wakeup(20 * 1000000ULL)`.
- On touch wakeup:
  - Read the touch pad value and log it with the threshold.
  - Blink LED (GPIO 21, active LOW) 3 times (200 ms on / 200 ms off per blink).
  - Increment `RTC_DATA_ATTR uint32_t touch_count` and log the total.
- On timer wakeup:
  - Log "Timer wakeup — no touch detected" with elapsed seconds.
  - Blink LED once slowly (500 ms).
- Recalibrate threshold on every timer wakeup (drift compensation).
- Return to deep sleep after feedback sequence.
- Log wakeup cause, touch value, and threshold on every wake.

## Hardware Dependencies

- Board-spec: board-specs/seeed/xiao-esp32s3.md (ESP32-S3, Touch T1 = GPIO 1 / D0, LED GPIO 21 active LOW)
- External: a wire or conductive pad connected to D0 (GPIO 1) acts as the touch sensor. No other components required.

## Power Budget

| State | Current | Duration |
|---|---|---|
| Deep sleep (touch sensing active) | ~130 µA | up to 20 s |
| Active (wake, blink, log) | ~68 mA | ~1.5 s |
| Target average | < 200 µA | — |

**Note**: Touch sensing during deep sleep consumes ~130 µA (touch controller stays active), higher than timer-only sleep (~14 µA). This is the inherent cost of touch wakeup.

## Success Criteria

- Touch on D0 wakes the device from deep sleep within 100 ms of contact.
- LED blinks 3× on touch wakeup; 1× on timer wakeup.
- `touch_count` increments on every touch wakeup; unchanged on timer wakeup.
- Threshold calibration produces consistent results across 3 power cycles.
- No false wakeups (phantom touches) in ambient conditions without physical contact.
