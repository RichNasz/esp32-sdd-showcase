# ==================== BOARD SELECTION ====================
# Change only this section, then regenerate the project
#
# Board: seeed-xiao-esp32s3          ← ACTIVE / DEFAULT
#
# =======================================================
# Institutional Memory: shared-specs/AIGenLessonsLearned.md — read before regenerating

# Coding Specification — Capacitive Touch Wakeup

## Architecture

Single-file firmware with a calibration-then-sleep loop. On first boot and on every timer
wakeup, the device samples the touch pad multiple times to establish a noise floor, derives a
threshold from the sample mean, stores the threshold in RTC memory, arms both touch and timer
wakeup sources, and enters deep sleep. On touch wakeup, calibration is skipped — the stored
RTC threshold is reapplied directly — the touch counter is incremented, the LED blinks a
distinct pattern, and the device re-enters deep sleep. Both wakeup sources are always armed
simultaneously.

## Key Constraints

- Touch pad: TOUCH_PAD_NUM1 (maps to GPIO 1 on XIAO ESP32S3). This is the only touch-capable
  pin accessible on the standard XIAO header.
- Calibration: 50 filtered samples, threshold set to 70% of the sample mean. This ratio
  distinguishes a real touch from idle noise while accommodating normal environmental variation.
- Dual wakeup: touch wakeup source and a 20-second backup timer are both armed before sleep.
  The timer fires when no touch occurs and triggers recalibration on the next cycle.
- LED feedback: touch wakeup produces a 3-blink pattern; timer wakeup produces a single slow
  blink. Distinct patterns are the primary visual diagnostic without a serial console.
- Persistence: touch_count (incremented on touch wakeup only) and rtc_threshold (set at
  calibration, reused on touch wakeup) must both survive deep sleep via RTC_DATA_ATTR.

## Preferred Libraries and APIs

Use the ESP-IDF touch sensor driver (driver/touch_sensor.h). The built-in driver handles
hardware filter initialization and provides filtered readings that suppress high-frequency
noise. Direct ADC access bypasses this filter and significantly increases false-trigger rates.

Use esp_sleep for wakeup source configuration and esp_sleep_get_touchpad_wakeup_status() to
identify which pad triggered wakeup. Use esp_sleep_get_wakeup_cause() to distinguish touch
wakeup from timer wakeup at the top of app_main.

## Non-Functional Requirements

- If calibration reads fail, the device must fall back to a safe hardcoded threshold rather
  than aborting. The device must always proceed to deep sleep — calibration failure must never
  block the sleep cycle.
- gpio_hold_dis() must be called before driving the LED GPIO on any wake from deep sleep, as
  the pad state may be held from the previous sleep cycle.
- USB-CDC requires a brief startup delay at the start of app_main before any logging, specific
  to the XIAO's native USB implementation.

## Gotchas

- ESP32-S3 touch sensor raw readings decrease when a finger is present (increased capacitance
  lengthens charge time, lowering the raw count). The threshold must be set below the idle
  baseline — not above it. The 70% calibration factor accounts for this inverse relationship.
- touch_pad_filter_start() must be called before touch_pad_read_filtered(). Calling the
  filtered read without starting the filter returns stale or zero values.
- Deep sleep current with touch wakeup enabled on the XIAO ESP32S3 is approximately 130 µA,
  substantially higher than timer-only deep sleep (~14 µA). This is expected hardware behaviour
  and must be documented in the README.
- TOUCH_PAD_NUM1 maps to GPIO 1 on ESP32-S3, but this mapping differs from earlier ESP32
  variants. Always verify against the board spec for the target chip.
