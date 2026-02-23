# Testing Specification — Capacitive Touch Wakeup

## Testing Philosophy

Automated testing using native ESP-IDF tools is strongly preferred.
Additional tools are only allowed with explicit justification and documented benefits.
Manual testing is used only when no practical automated alternative exists.

---

## Automated Tests

### T-A1 — Zero-Warning Build

**Tool**: `idf.py` (native)

```sh
cd examples/capacitive-touch-wakeup
idf.py set-target esp32s3 && idf.py build
```

Pass: exit code 0, zero compiler warnings.

---

### T-A2 — Binary Size Check

**Tool**: `idf.py` (native)

```sh
idf.py size | grep "Total binary size"
```

Pass: < 1 MB.

---

### T-A3 — Timer Wakeup via Wokwi Simulation

**Tool**: Wokwi CLI — justification: the 20-second backup timer wakeup path is testable in simulation; only the touch wakeup path requires physical hardware.

Prerequisites: Wokwi CLI installed; `wokwi.toml` + `diagram.json` at example root (no touch pad wired — forces timer wakeup path).

```sh
wokwi-cli --timeout 30000 --expect "Timer wake" .
```

Pass: `Timer wake` appears in simulated serial output within 30 seconds.

---

## Manual Tests

*Capacitive touch sensing is a physical human-interface phenomenon. Touch detection quality (threshold, false-positive rate) and the deep-sleep touch wakeup mechanism require real ESP32-S3 hardware.*

### T-M1 — Touch Wakeup from Deep Sleep

**Why manual**: Physical finger contact activates the capacitive sense circuit. Simulation cannot model finger capacitance or threshold drift.

Hardware: XIAO ESP32S3 + a wire or conductive pad connected to D0 (GPIO 1).

1. Flash to XIAO ESP32S3: `idf.py set-target esp32s3 && idf.py build flash`
2. Open serial monitor: `idf.py monitor`
3. Wait for device to enter deep sleep (confirm "Sleeping" log).
4. Touch the wire/pad on D0.
5. Confirm wake within 100 ms of touch.
6. Confirm serial shows touch wakeup log with pad number, touch count, and threshold.
7. Confirm LED blinks exactly 3×.

Pass: wakeup on touch within 100 ms; 3 LED blinks; touch_count increments.

---

### T-M2 — Threshold Calibration Stability

**Why manual**: Calibration accuracy depends on real sensor ambient noise levels.

1. Run T-M1 hardware setup.
2. Power-cycle the board 3 times.
3. On each boot, confirm calibration log shows a consistent mean (within ±10% across power cycles) in the same ambient conditions.
4. Confirm no false touch wakeup occurs when the pad is not touched.

Pass: calibration stable within ±10%; no false wakeups in idle environment.

---

### T-M3 — Timer Wakeup (No Touch)

**Why manual**: Confirms the backup 20-second timer wakeup fires when no touch occurs.

1. Flash and monitor.
2. Do not touch the pad.
3. After 20 seconds, confirm "Timer wake" log and single slow LED blink.
4. Confirm device recalibrates and returns to sleep.

Pass: timer wakeup fires at ~20 s; single LED blink; device sleeps again.
