# ==================== BOARD SELECTION ====================
# Change only this section, then regenerate the project
#
# Board: seeed-xiao-esp32s3          ← ACTIVE / DEFAULT
#
# =======================================================
# Institutional Memory: shared-specs/AIGenLessonsLearned.md — read before regenerating

# Coding Specification — Deep Sleep Timer Node

## Architecture

Minimal single-shot firmware. App_main runs once per wake cycle: increment the boot counter,
detect wakeup cause, log status, blink the LED, then immediately enter deep sleep. There
are no FreeRTOS tasks beyond app_main's implicit task, no event loops, and no persistent
connections. The entire active window is synchronous and linear.

## Key Constraints

- Sleep duration: 15 seconds.
- Active window budget: 200 ms maximum. This includes ESP-IDF boot time, LED blink, logging,
  and serial flush. Anything beyond 200 ms active time indicates a regression.
- Persistence: RTC_DATA_ATTR is the correct mechanism for the boot counter. NVS is
  unnecessary overhead for a single integer that may reset on power-cycle.
- The boot counter must be incremented as the first action in app_main, before any
  conditional branching or logging, so every wake cycle produces an accurate count.

## Preferred Libraries and APIs

Use esp_sleep for wakeup configuration and esp_sleep_get_wakeup_cause() to distinguish
first boot from timer wakeup. Use gpio for LED feedback — LEDC is unnecessary overhead
for a single binary blink.

## Non-Functional Requirements

- Wakeup cause must be checked on every boot. First power-on and timer wakeup are distinct
  states and should produce distinct log messages.
- Unrecognised wakeup causes must log a warning and proceed to sleep — never abort. The
  deep-sleep loop must be robust to unexpected causes.
- A brief delay is needed after app_main entry for USB-CDC to enumerate before the first
  log line is emitted. This is specific to the XIAO ESP32S3's native USB implementation.

## Gotchas

- gpio_hold_dis() must be called before driving the LED GPIO on each wake. Deep sleep can
  hold GPIO pad states, and attempting to drive a held pin has no effect.
- RTC_DATA_ATTR variables survive deep sleep but reset on a full power-cycle or power-on
  reset. This is expected and correct behaviour; document it in the README.
