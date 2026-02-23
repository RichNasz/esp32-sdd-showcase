# ==================== BOARD SELECTION ====================
# Change only this section, then regenerate the project
#
# Board: seeed-xiao-esp32s3          ← ACTIVE / DEFAULT (your XIAO ESP32S3 boards)
# # Board: adafruit-huzzah32         ← Uncomment this line to switch to HUZZAH32
#
# =======================================================
# Institutional Memory: shared-specs/AIGenLessonsLearned.md — read before regenerating

# Coding Specification — Breathing LED (Multi-Board)

## Architecture

Single-file firmware with no FreeRTOS tasks beyond app_main's implicit task. Board selection
is a compile-time Kconfig choice; the active board determines GPIO number, LEDC speed mode,
and LED polarity. The breathing effect is entirely interrupt-driven: app_main starts the
first fade and enters an idle loop, and all subsequent fades are triggered by the LEDC
fade-done callback.

## Board Abstraction

Board-specific constants (GPIO number, LEDC speed mode, active polarity) must be resolved
at compile time via Kconfig macros — never at runtime. The active-LOW nature of the XIAO
LED requires duty-cycle inversion: DUTY_ON and DUTY_OFF must be defined in terms of the
board's polarity, not as raw 0 and max. The breathing sequence always starts dark and
brightens first, regardless of board.

## Key Constraints

- LEDC: 5 kHz PWM frequency, 13-bit duty resolution. These values produce flicker-free
  fading visible to the human eye.
- Breathing period: 4 seconds total (2 s up, 2 s down).
- ESP32-S3 supports only low-speed LEDC mode; ESP32 supports high-speed. The board
  selection must map to the correct mode or the build will fail at runtime.
- Kconfig.projbuild must live in main/, not the project root. See AIGenLessonsLearned.md.

## Preferred Libraries and APIs

Use the ESP-IDF LEDC driver's fade subsystem (ledc_fade_func_install and the callback
registration API) rather than manually stepping duty in a timer or task. This eliminates
CPU involvement during fading and produces perfectly smooth results.

## Non-Functional Requirements

- The IRAM_ATTR fade callback must not access flash. Any logging inside the callback must
  use DRAM-safe macros (ESP_DRAM_LOGI). ESP_ERROR_CHECK must not be used inside the callback
  as it calls abort(), which is unsafe from ISR context.
- All ESP-IDF calls outside the callback use ESP_ERROR_CHECK.
- The XIAO ESP32S3 uses native USB CDC for serial; CONFIG_ESP_CONSOLE_USB_CDC=y is required
  in sdkconfig.defaults when XIAO is the active board.

## Gotchas

- LEDC_HIGH_SPEED_MODE is unavailable on ESP32-S3; accidentally using it causes a
  link-time error, not a compile-time warning. The Kconfig guard must select the correct
  speed mode for each board.
- The two boards have opposite LED polarity. Failing to invert duty correctly results in
  the LED being off when it should be on and vice versa.

## File Layout (non-standard files)

- main/Kconfig.projbuild — required for the multi-board Kconfig choice
