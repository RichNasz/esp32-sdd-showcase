<!-- ================================================
     AGENT-GENERATED — DO NOT EDIT BY HAND
     Generated from specs/ using esp32-sdd-documentation-generator skill
     Date: 2026-02-22 | Agent: Claude Code
     ================================================ -->

# 08 · Capacitive Touch Wakeup — Touch-Driven Interfaces

> **Human-interface deep sleep.** Wakes the ESP32 from deep sleep on a fingertip touch — enabling zero-standby-power touch interfaces for panels, enclosures, and wearables.

## What It Demonstrates

- ESP32 capacitive touch sensor peripheral (up to 10 touch pads)
- Touch-pad wakeup from deep sleep via `esp_sleep_enable_touchpad_wakeup()`
- Touch pad calibration with automatic threshold computation
- Multi-pad detection: identifying which pad triggered the wakeup event

## Target Hardware

| Item | Detail |
|---|---|
| Board | ESP32-DevKitC or Adafruit HUZZAH32 |
| Touch pads | GPIO 4 (T0), GPIO 2 (T2), GPIO 15 (T3) — wire to copper foil pads |
| Sleep current | ~10 µA (touch sensor peripheral stays active during sleep) |
| Wakeup latency | < 1 ms from touch to firmware execution |

## Build & Flash

```sh
cd examples/08-capacitive-touch-wakeup
idf.py set-target esp32
idf.py build flash monitor
```

## Expected Output

```
I (288) touch_wake: Calibrating... baseline T0=648 T2=621 T3=634
I (310) touch_wake: Thresholds set — T0:324 T2:310 T3:317 (50% drop)
I (312) touch_wake: Ready. Touch any pad to wake from deep sleep.
--- deep sleep ---
--- wakeup ---
I (290) touch_wake: Woke on pad T2 (GPIO 2) | raw value: 308
```

## Key Concepts

- `touch_pad_init()` + `touch_pad_set_fsm_mode(TOUCH_FSM_MODE_TIMER)`
- `touch_pad_filter_start(10)` — 10 ms IIR filter for noise immunity
- `esp_sleep_enable_touchpad_wakeup()` + `esp_sleep_get_touchpad_wakeup_status()`
- Threshold formula: `threshold = calibrated_baseline × 0.50` (50% capacitance drop = definitive touch)

## Design Notes

- Use copper foil or PCB copper pour pads, minimum 1 cm² for reliable sensing through 3 mm acrylic.
- Keep touch pad traces short and away from power lines; use a ground guard ring around each trace.
- `TOUCH_PAD_NO_CHANGE` reported value decreases on touch (inverse of intuition).

## Spec Files

- [FunctionalSpec.md](FunctionalSpec.md)
- [CodingSpec.md](CodingSpec.md)
