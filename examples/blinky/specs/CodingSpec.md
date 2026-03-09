<!--
BOARD SELECTION APPROACH
LED GPIO and polarity are Kconfig int symbols (EXAMPLE_LED_GPIO, EXAMPLE_LED_ACTIVE_LEVEL)
set via per-target sdkconfig.defaults.<target> files. No menuconfig step is required.

Institutional Memory: shared-specs/AIGenLessonsLearned.md — read before regenerating
-->

# Coding Specification — Breathing LED (Multi-Board)

## Architecture

Single-file firmware with no FreeRTOS tasks beyond app_main's implicit task. Board selection
is driven by per-target `sdkconfig.defaults.<target>` files; the active target determines
GPIO number, LEDC speed mode, and LED polarity at compile time. The breathing effect is
entirely interrupt-driven: app_main starts the first fade and enters an idle loop, and all
subsequent fades are triggered by the LEDC fade-done callback.

## Board Abstraction

Board-specific constants (GPIO number, active polarity) are Kconfig int symbols
(`EXAMPLE_LED_GPIO`, `EXAMPLE_LED_ACTIVE_LEVEL`) injected by per-target
`sdkconfig.defaults.<target>` files. LEDC speed mode is resolved via
`#ifdef CONFIG_IDF_TARGET_ESP32` — this is a genuine SoC architectural difference,
not a board choice. The active-LOW nature of the XIAO LED requires duty-cycle inversion:
DUTY_ON and DUTY_OFF must be defined in terms of the polarity, not as raw 0 and max.
The breathing sequence always starts dark and brightens first, regardless of board.

## Key Constraints

- LEDC: 5 kHz PWM frequency, 13-bit duty resolution. These values produce flicker-free
  fading visible to the human eye.
- Breathing period: 4 seconds total (2 s up, 2 s down).
- ESP32-S3 and ESP32-C5 support only low-speed LEDC mode; ESP32 supports high-speed. Use
  `#ifdef CONFIG_IDF_TARGET_ESP32` to select the correct mode — this is the canonical
  ESP-IDF idiom for this SoC architectural difference.
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
  causes a link-time error, not a compile-time warning. The `#ifdef CONFIG_IDF_TARGET_ESP32`
  guard is the correct resolution — not a Kconfig board choice.
- The HUZZAH32 LED is active HIGH; the XIAO ESP32S3, XIAO ESP32-C5, and XIAO ESP32-C6 LEDs are active LOW.
  Failing to invert duty correctly results in the LED being off when it should be on and vice versa.
- The XIAO ESP32-C5 console config is CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y, not
  CONFIG_ESP_CONSOLE_USB_CDC=y. These are different peripherals; using the wrong one
  produces no monitor output.
- `idf.py set-target <chip>` is the only step needed when switching boards — the
  per-target `sdkconfig.defaults.<target>` file automatically injects the correct LED GPIO,
  polarity, and console config. No menuconfig step is required.

## File Layout (non-standard files)

- main/Kconfig.projbuild — defines EXAMPLE_LED_GPIO and EXAMPLE_LED_ACTIVE_LEVEL int symbols
- sdkconfig.defaults.<target> — per-target overrides for GPIO, polarity, and console config
