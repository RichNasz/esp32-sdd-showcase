<!--
DEFAULT BOARD FOR CODE GENERATION
Sets the Kconfig default in the generated project. After generation, board
selection is a compile-time Kconfig choice — change it there without touching this spec.

  Default: seeed-xiao-esp32s3   (options: adafruit-huzzah32, seeed-xiao-esp32c5, seeed-xiao-esp32c6)

Institutional Memory: shared-specs/AIGenLessonsLearned.md — read before regenerating
-->

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
- ESP32-S3 and ESP32-C5 support only low-speed LEDC mode; ESP32 supports high-speed. The
  board selection must map to the correct mode or the build will fail at runtime.
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
  in sdkconfig.defaults when XIAO S3 is the active board.
- The XIAO ESP32-C5 uses the USB Serial/JTAG controller (same family as C3/C6, not S3's
  USB OTG); CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y is required in sdkconfig.defaults when
  XIAO C5 is the active board.

## Gotchas

- LEDC_HIGH_SPEED_MODE is unavailable on ESP32-S3 and ESP32-C5; accidentally using it
  causes a link-time error, not a compile-time warning. The Kconfig guard must select the
  correct speed mode for each board.
- The HUZZAH32 LED is active HIGH; the XIAO ESP32S3, XIAO ESP32-C5, and XIAO ESP32-C6 LEDs are active LOW.
  Failing to invert duty correctly results in the LED being off when it should be on and vice versa.
- The XIAO ESP32-C5 console config is CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y, not
  CONFIG_ESP_CONSOLE_USB_CDC=y. These are different peripherals; using the wrong one
  produces no monitor output.
- Switching boards requires two steps in this order: first `idf.py set-target <chip>`
  (which clears the stale sdkconfig), then selecting the correct board in
  `idf.py menuconfig`. Skipping set-target leaves the old sdkconfig in place — the
  binary will drive the old board's GPIO, the LED will not respond, and the startup
  log will show the wrong GPIO number on the "Board GPIO=..." line.

## File Layout (non-standard files)

- main/Kconfig.projbuild — required for the multi-board Kconfig choice
