# ==================== BOARD SELECTION ====================
# Change only this section, then regenerate the project
#
# Board: seeed-xiao-esp32s3          ← ACTIVE / DEFAULT (your XIAO ESP32S3 boards)
# # Board: adafruit-huzzah32         ← Uncomment this line to switch to HUZZAH32
#
# =======================================================
# Institutional Memory: shared-specs/AIGenLessonsLearned.md — read before regenerating

# Coding Specification — Breathing LED (Multi-Board)

## Board Abstraction
- LED GPIO is NOT hard-coded. It is derived from the active board-spec via Kconfig.
- Kconfig.projbuild exposes a `BOARD_SELECT` choice (default: BOARD_SEEED_XIAO_ESP32S3).
- sdkconfig.defaults sets the default to match `# Board:` comment above.
- main.c resolves LED_GPIO at compile time via CONFIG_BOARD_* macros.
- `LED_ACTIVE_LOW` compile-time flag: `0` for HUZZAH32 (GPIO HIGH = LED on), `1` for XIAO (GPIO LOW = LED on).
- `DUTY_ON` macro: resolves to `0` when `LED_ACTIVE_LOW=1` (XIAO), or `(1 << LEDC_RESOLUTION) - 1` when `LED_ACTIVE_LOW=0` (HUZZAH32).
- `DUTY_OFF` macro: resolves to `(1 << LEDC_RESOLUTION) - 1` when `LED_ACTIVE_LOW=1`, or `0` when `LED_ACTIVE_LOW=0`.
- Breathing always starts dark → bright → dark: initial channel duty = `DUTY_OFF`; first fade targets `DUTY_ON`.

## LEDC Configuration
- Timer: LEDC_TIMER_0, high-speed mode (LEDC_HIGH_SPEED_MODE) for ESP32;
  low-speed mode (LEDC_LOW_SPEED_MODE) for ESP32-S3 (no high-speed mode).
- Frequency: 5000 Hz, 13-bit duty resolution.
- Channel: LEDC_CHANNEL_0.
- Timer clock: LEDC_AUTO_CLK (let the driver select the optimal source).
- Channel intr_type: LEDC_INTR_DISABLE (fade callbacks replace hardware interrupts).
- Channel hpoint: 0 (standard PWM phase alignment).
- Use ledc_fade_func_install() + ledc_set_fade_with_time() for smooth fades.
- Register a ledc_cbs_t callback (ledc_cb_register) for fade-done events.
- On fade-done: callback toggles between `DUTY_ON` and `DUTY_OFF` — never between raw 0 and max.
- Variable `fading_to_on` (bool) tracks current fade direction; toggled in the callback.
- Breathing sequence: DUTY_OFF → DUTY_ON → DUTY_OFF → … (always starts dark, then brightens).
- After starting the first fade, app_main enters an infinite `vTaskDelay(pdMS_TO_TICKS(1000))` loop;
  all subsequent breathing work is driven by the fade callback.

## Logging
- Tag: "blinky"
- On init: log GPIO number, frequency, resolution, and active polarity (`active-HIGH` or `active-LOW`).
- On init: log total breathing period in ms (FADE_TIME_MS * 2).
- On each fade start inside the callback: log fade direction ("up" or "down") and the target duty
  value (DUTY_ON or DUTY_OFF resolved to its raw integer).

## USB-CDC (XIAO ESP32S3)
- The XIAO ESP32S3 uses native USB for its serial console; there is no USB-to-UART chip.
- `CONFIG_ESP_CONSOLE_USB_CDC=y` must be set in sdkconfig.defaults when targeting esp32s3.
- Without this setting, `idf.py monitor` shows no output on XIAO hardware.
- `CONFIG_ESP_CONSOLE_USB_CDC=y` is active (uncommented) in sdkconfig.defaults because XIAO is the default board.
  Comment it out — and uncomment any UART console config — when switching to HUZZAH32.

## Agent-Generated Headers (required on every generated file)
- All generated files must carry the standard agent-generated header on line 1.
- Use the format appropriate to the file type:
  - `.c` files: `/* AGENT-GENERATED — do not edit by hand; regenerate via esp32-sdd-full-project-generator */`
  - `CMakeLists.txt`, `sdkconfig.defaults`, `.gitignore`, `Kconfig.projbuild`: `# AGENT-GENERATED — do not edit by hand; regenerate via esp32-sdd-full-project-generator`
  - `README.md`: the full HTML comment block (see existing README for format)

## Error Handling
- Wrap every ESP-IDF call with ESP_ERROR_CHECK.
- Exception: `ledc_set_fade_with_time()` and `ledc_fade_start()` called inside the IRAM_ATTR
  fade callback must NOT use ESP_ERROR_CHECK — abort() from ISR context is unsafe. Call them
  bare and ignore the return value inside the callback.
- For logging inside the IRAM_ATTR callback, use `ESP_DRAM_LOGI` (not `ESP_LOGI`). This macro
  places the format string in DRAM rather than flash, making it safe to access from IRAM context.

## File Layout (locked — matches example-project-structure-spec.md)
- main/main.c
- main/CMakeLists.txt
- main/Kconfig.projbuild
- CMakeLists.txt
- sdkconfig.defaults
- .gitignore
- README.md
