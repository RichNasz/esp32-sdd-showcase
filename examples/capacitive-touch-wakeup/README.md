<!-- ================================================
     AGENT-GENERATED — DO NOT EDIT BY HAND
     Generated from specs/ using esp32-sdd-documentation-generator skill
     Date: 2026-03-13 | Agent: Claude Code
     ================================================ -->

# Capacitive Touch Wakeup — Touch-Driven Interfaces

> **Zero-power human interface.** Sleeps until a finger touch wakes the device — no button, no polling, no active power draw during sleep.

## Overview

The device sleeps until a capacitive touch on pad T1 (GPIO 1 / D0) wakes it instantly.
On wake, it blinks the LED three times, logs the touch reading and threshold over serial,
increments a persistent touch counter, and returns to sleep. A backup 20-second timer wakeup
prevents indefinite sleep if the pad is never touched — on timer wakeup the device recalibrates
the threshold to compensate for drift and blinks the LED once (slow).

The threshold is calibrated automatically on first boot: 50 filtered samples in idle state,
mean × 0.7. The ESP32-S3 touch sensor raw reading *decreases* when a finger is present
(increased capacitance), so the threshold is set *below* the idle baseline.

> **Power note**: Touch sensing during deep sleep consumes ~130 µA (the touch controller stays
> active). This is substantially higher than timer-only deep sleep (~14 µA) and is the inherent
> cost of touch wakeup.

## Board

| Item | Detail |
|---|---|
| Board | Seeed XIAO ESP32S3 |
| Target chip | ESP32-S3 |
| ESP-IDF | v5.5.x |
| Touch pad | TOUCH_PAD_NUM1 → GPIO 1 (D0) |
| LED | GPIO 21, active LOW |
| Touch hardware | Wire or conductive pad connected to D0 — no other components needed |

## Build & Flash

```sh
cd examples/capacitive-touch-wakeup
idf.py set-target esp32s3
idf.py build flash monitor
```

Connect a short wire to the D0 header pin. Touch the wire with your finger to trigger wakeup.

## Expected Serial Output

```
I (312) touch_wakeup: First boot — calibrating touch threshold...
I (315) touch_wakeup: Calibration: mean=8000 threshold=5600 (70%)
I (317) touch_wakeup: Entering deep sleep (touch + 20s timer armed)...

--- (finger touch on D0) ---

I (312) touch_wakeup: Touch wakeup! pad=T1 value=4200 threshold=5600 count=1
I (315) touch_wakeup: LED: 3 blinks
I (915) touch_wakeup: Entering deep sleep (touch + 20s timer armed)...

--- (20 seconds, no touch) ---

I (312) touch_wakeup: Timer wakeup — no touch detected. Recalibrating...
I (315) touch_wakeup: Calibration: mean=7950 threshold=5565 (70%)
I (317) touch_wakeup: LED: 1 slow blink
I (817) touch_wakeup: Entering deep sleep (touch + 20s timer armed)...
```

## Key Concepts

- `esp_sleep_enable_touchpad_wakeup()` — the CPU is fully off; only the touch controller
  (~130 µA) stays active during sleep
- `esp_sleep_get_wakeup_cause()` / `esp_sleep_get_touchpad_wakeup_status()` — distinguish
  touch wakeup from timer wakeup and identify which pad fired
- `touch_pad_filter_start()` must be called before `touch_pad_read_filtered()` — stale or zero
  values result if the filter is not started first
- Calibration: 50 filtered samples, threshold = mean × 0.7 (30% drop signals a real touch)
- Raw readings *decrease* on touch (inverse of intuition) — threshold must be below the idle
  baseline, not above it
- `RTC_DATA_ATTR` for `touch_count` (persistent touch counter) and `rtc_threshold` (reused
  on touch wakeup to skip re-calibration)
- `gpio_hold_dis()` before driving LED on each wake cycle

## Power Budget

| State | Current | Duration |
|---|---|---|
| Deep sleep (touch active) | ~130 µA | up to 20 s |
| Active (wake, blink, log) | ~68 mA | ~1.5 s |
| Target average | < 200 µA | — |

## Testing

Testing philosophy: automated first, manual only when hardware observation is unavoidable.

### Automated Tests

1. **T-A1 — Zero-Warning Build**

   ```sh
   cd examples/capacitive-touch-wakeup
   idf.py set-target esp32s3 && idf.py build
   ```

   Pass: exit code 0, zero compiler warnings.

2. **T-A2 — Binary Size Check**

   ```sh
   idf.py size | grep "Total binary size"
   ```

   Pass: < 1 MB.

3. **T-A3 — Timer Wakeup via Wokwi Simulation**

   Justification: the 20-second backup timer wakeup path is testable in simulation; only
   the touch wakeup path requires physical hardware.

   Prerequisites: [Install Wokwi CLI](https://docs.wokwi.com/wokwi-ci/getting-started);
   `wokwi.toml` + `diagram.json` at example root (no touch pad wired — forces timer path).

   ```sh
   wokwi-cli --timeout 30000 --expect "Timer wake" .
   ```

   Pass: `Timer wake` appears in simulated serial output within 30 seconds.

### Manual Tests — hardware required

**T-M1 — Touch Wakeup from Deep Sleep**

Why manual: physical finger contact activates the capacitive sense circuit. Simulation
cannot model finger capacitance or threshold drift.

Hardware: XIAO ESP32S3 + a wire or conductive pad connected to D0 (GPIO 1).

1. Flash: `idf.py set-target esp32s3 && idf.py build flash monitor`
2. Wait for the device to enter deep sleep (confirm "Entering deep sleep" in serial log).
3. Touch the wire/pad connected to D0.
4. Confirm wake occurs within 100 ms of touch.
5. Confirm serial shows touch wakeup log with pad number, touch value, threshold, and count.
6. Confirm LED blinks exactly 3 times.

Pass: wakeup on touch within 100 ms; 3 LED blinks; `touch_count` increments.

---

**T-M2 — Threshold Calibration Stability**

Why manual: calibration accuracy depends on real sensor ambient noise levels.

1. Run T-M1 hardware setup.
2. Power-cycle the board 3 times.
3. On each boot, confirm calibration log shows a consistent mean (within ±10% across power
   cycles in the same ambient conditions).
4. Confirm no false touch wakeup occurs when the pad is not touched.

Pass: calibration stable within ±10%; no false wakeups in idle environment.

---

**T-M3 — Timer Wakeup (No Touch)**

Why manual: confirms the backup 20-second timer wakeup fires when no touch occurs.

1. Flash and monitor.
2. Do not touch the pad.
3. After 20 seconds, confirm "Timer wake" log and a single slow LED blink.
4. Confirm device recalibrates and returns to sleep.

Pass: timer wakeup fires at ~20 s; single LED blink; device sleeps again.

## Spec Files

- [specs/FunctionalSpec.md](specs/FunctionalSpec.md)
- [specs/CodingSpec.md](specs/CodingSpec.md)
- [specs/TestSpec.md](specs/TestSpec.md)
