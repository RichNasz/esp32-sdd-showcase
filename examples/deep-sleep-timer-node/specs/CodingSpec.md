# ==================== BOARD SELECTION ====================
# Change only this section, then regenerate the project
#
# Primary boards (sdkconfig.defaults.<target> configured for):
#   esp32    → Adafruit HUZZAH32          (GPIO 13, active HIGH)
#   esp32s3  → YEJMKJ ESP32-S3-DevKitC-1-N16R8  (GPIO 48, WS2812)
#   esp32c5  → Seeed XIAO ESP32-C5        (GPIO 27, active LOW)
#   esp32c6  → Espressif ESP32-C6-DevKitC-1-N8  (GPIO 8,  WS2812)
#
# Secondary boards (require menuconfig override after set-target):
#   esp32s3  → Seeed XIAO ESP32S3         (GPIO 21, active LOW, USB CDC)
#   esp32c6  → Seeed XIAO ESP32-C6        (GPIO 15, active LOW)
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

- Sleep duration: 5 seconds.
- Active window budget: 200 ms maximum. This includes ESP-IDF boot time, LED blink, logging,
  and serial flush. Anything beyond 200 ms active time indicates a regression.
  The 200 ms budget is most constraining on USB-CDC boards (XIAO ESP32S3 needs ~50 ms for
  USB enumeration); WS2812 primary boards (USB Serial/JTAG or UART bridge) have no enumeration
  delay and meet the budget more readily. Keep all targets under 200 ms.
- Persistence: RTC_DATA_ATTR is the correct mechanism for the boot counter. NVS is
  unnecessary overhead for a single integer that may reset on power-cycle.
- The boot counter must be incremented as the first action in app_main, before any
  conditional branching or logging, so every wake cycle produces an accurate count.

## LED Architecture

Two LED code paths, selected at compile time by `CONFIG_EXAMPLE_LED_WS2812`:

**`#if CONFIG_EXAMPLE_LED_WS2812` — WS2812 RMT path:**
- `led_init()` configures the RMT channel and bytes encoder (10 MHz resolution, GRB byte order).
  Clears the LED to dark immediately after init regardless of prior state.
- `led_heartbeat()` selects a random palette index via the hardware RNG (`esp_random()`), writes
  the chosen colour for 100 ms, then writes `(0,0,0)` for 100 ms before returning.
- Colour palette (6 entries, each with at least one channel at 64/255; no channel above 64/255):
  red (64,0,0), green (0,64,0), blue (0,0,64), cyan (0,64,64), magenta (64,0,64), amber (64,32,0).
  All entries meet the 64/255 minimum brightness floor from `shared-specs/AIGenLessonsLearned.md`.
- `esp_random()` reads directly from the hardware entropy source — no seeding, no srand() call.

**`#else` — Simple GPIO path:**
- `led_init()` calls `gpio_hold_dis()` to clear any deep-sleep hold on the LED GPIO, configures
  the pin as output, and drives it to the inactive level.
- `led_heartbeat()` sets the GPIO to its active level for 100 ms, then back to inactive for 100 ms.
- No PWM, no LEDC, no RMT — a brief binary blink requires none of it.

`gpio_hold_dis()` must be called before driving the LED GPIO in both paths. Deep sleep holds
GPIO pad states from the previous cycle; attempting to drive a held pin has no effect.

## Kconfig Symbols

Three symbols required in `main/Kconfig.projbuild`, inside `menu "Example Configuration"`:

- `EXAMPLE_LED_GPIO` (int, range 0–48): GPIO driving the onboard LED. Default is the primary
  board's GPIO; overridden per target via `sdkconfig.defaults.<target>`.
- `EXAMPLE_LED_WS2812` (bool, default n): when enabled, selects the WS2812 RMT code path.
  `EXAMPLE_LED_ACTIVE_LEVEL` is ignored when this symbol is enabled. Enable for WS2812 primary
  boards via `sdkconfig.defaults.<target>`; secondary GPIO-LED boards leave it disabled.
- `EXAMPLE_LED_ACTIVE_LEVEL` (int, range 0–1, default 0): logic level that turns the GPIO LED ON.
  0 = active LOW, 1 = active HIGH. Not used when `EXAMPLE_LED_WS2812` is enabled.

## Component Dependencies

`main/CMakeLists.txt REQUIRES` must always include `esp_driver_rmt` — even on targets where
`EXAMPLE_LED_WS2812=n`. This avoids conditional CMake complexity and is harmless when the
`#if CONFIG_EXAMPLE_LED_WS2812` guard compiles out all RMT code paths.

## Preferred Libraries and APIs

Use `esp_sleep` for wakeup configuration and `esp_sleep_get_wakeup_cause()` to distinguish
first boot from timer wakeup. Use `gpio` for simple GPIO LED feedback — LEDC is unnecessary
overhead for a single binary blink. Use `esp_random()` for WS2812 colour selection (hardware
entropy, no seeding required).

## Non-Functional Requirements

- Wakeup cause must be checked on every boot. First power-on and timer wakeup are distinct
  states and should produce distinct log messages.
- Unrecognised wakeup causes must log a warning and proceed to sleep — never abort. The
  deep-sleep loop must be robust to unexpected causes.
- A brief delay is needed after app_main entry for USB-CDC to enumerate before the first
  log line is emitted. This applies only to XIAO ESP32S3 (USB-CDC). Other boards may omit it.

## Gotchas

- `gpio_hold_dis()` must be called before driving the LED GPIO on each wake. Deep sleep can
  hold GPIO pad states, and attempting to drive a held pin has no effect.
- RTC_DATA_ATTR variables survive deep sleep but reset on a full power-cycle or power-on
  reset. This is expected and correct behaviour; document it in the README.
- WS2812 colour values of 8/255 are effectively invisible under normal room lighting and
  indistinguishable from a hardware fault. Always use 64/255 as the brightness floor
  (see `shared-specs/AIGenLessonsLearned.md`).
