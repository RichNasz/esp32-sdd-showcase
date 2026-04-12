<!--
BOARD SELECTION APPROACH
LED GPIO, polarity, and LED type are Kconfig symbols set via per-target
sdkconfig.defaults.<target> files. No menuconfig step is required for listed boards.

Two code paths exist:
  - LEDC path (CONFIG_EXAMPLE_LED_WS2812=n): hardware PWM fade, interrupt-driven callback
  - WS2812 path (CONFIG_EXAMPLE_LED_WS2812=y): RMT peripheral, software brightness loop

Institutional Memory: shared-specs/AIGenLessonsLearned.md — read before regenerating
-->

# Coding Specification — Breathing LED (Multi-Board)

## Architecture

Single-file firmware with no FreeRTOS tasks beyond app_main's implicit task. Board selection
is driven by per-target `sdkconfig.defaults.<target>` files; the active target determines
GPIO number, LED type (simple vs WS2812), and LED polarity at compile time. Two breathing
implementations exist, selected by the `EXAMPLE_LED_WS2812` Kconfig bool:

- **LEDC path** (WS2812=n): entirely interrupt-driven. app_main starts the first fade and
  enters an idle loop; all subsequent fades are triggered by the LEDC fade-done callback.
- **WS2812 path** (WS2812=y): software loop in app_main. Each iteration sends one RMT frame
  via `ws2812_write(r, g, b)` then delays `vTaskDelay(pdMS_TO_TICKS(2000/256))` to step
  white brightness 0→255→0 over 4 seconds total.

## Board Abstraction

Board-specific constants are Kconfig symbols injected by per-target `sdkconfig.defaults.<target>`:
- `EXAMPLE_LED_GPIO` (int): GPIO number for both LEDC and RMT output.
- `EXAMPLE_LED_ACTIVE_LEVEL` (int, 0 or 1): polarity for LEDC path only (not used for WS2812).
- `EXAMPLE_LED_WS2812` (bool): selects code path. n = LEDC; y = RMT/WS2812.

LEDC speed mode is resolved via `#ifdef CONFIG_IDF_TARGET_ESP32` — a genuine SoC architectural
difference, not a board choice. The active-LOW nature of the XIAO LED requires duty-cycle
inversion: DUTY_ON and DUTY_OFF must be defined in terms of the polarity, not as raw 0 and max.
The breathing sequence always starts dark and brightens first, regardless of board.

## WS2812 Code Path

When `CONFIG_EXAMPLE_LED_WS2812=y`:

- Use `esp_driver_rmt` component (rmt_tx.h, rmt_encoder.h).
- RMT resolution: 10 MHz (1 tick = 100 ns). This matches the reference implementation in
  `examples/esp-now-low-power-mesh/main/mesh_common.c` lines 36–72 — reuse that channel
  init and `ws2812_write()` pattern verbatim (adapted to use `CONFIG_EXAMPLE_LED_GPIO`).
- WS2812B NRZ timing:
  - Bit-0: T0H = 3 ticks (300 ns) HIGH, T0L = 9 ticks (900 ns) LOW
  - Bit-1: T1H = 9 ticks (900 ns) HIGH, T1L = 3 ticks (300 ns) LOW
- GRB byte order (not RGB). `ws2812_write(r, g, b)` must pack as `{g, r, b}`.
- After each RMT frame: call `rmt_tx_wait_all_done(chan, pdMS_TO_TICKS(100))` before next write.
- Breathing loop: step R=G=B from 0→255 (2 s up) then 255→0 (2 s down), 256 steps per ramp,
  `vTaskDelay(pdMS_TO_TICKS(2000 / 256))` per step. Loop runs forever in app_main.
- Cap white brightness at 64/255 max to avoid eye-blasting intensity on a bare LED.
- Log "WS2812 RMT channel init on GPIO %d" at INFO level before the loop.

## Key Constraints

- LEDC: 5 kHz PWM frequency, 13-bit duty resolution. These values produce flicker-free
  fading visible to the human eye.
- Breathing period: 4 seconds total (2 s up, 2 s down) — applies to both LEDC and WS2812 paths.
- ESP32-S3 and ESP32-C5 support only low-speed LEDC mode; ESP32 supports high-speed. Use
  `#ifdef CONFIG_IDF_TARGET_ESP32` to select the correct mode — this is the canonical
  ESP-IDF idiom for this SoC architectural difference.
- Kconfig.projbuild must live in main/, not the project root. See AIGenLessonsLearned.md.
- `esp_driver_rmt` must always be listed in main/CMakeLists.txt REQUIRES, even on non-WS2812
  targets — the `#if CONFIG_EXAMPLE_LED_WS2812` guard prevents actual code from compiling,
  but the component reference is harmless and avoids conditional CMake complexity.

## Preferred Libraries and APIs

- LEDC path: ESP-IDF LEDC driver fade subsystem (`ledc_fade_func_install`, callback registration).
- WS2812 path: ESP-IDF RMT driver (`rmt_new_tx_channel`, `rmt_new_bytes_encoder`,
  `rmt_transmit`, `rmt_tx_wait_all_done`). Reference: mesh_common.c lines 36–72.

## Non-Functional Requirements

- The IRAM_ATTR LEDC fade callback must not access flash. Any logging inside the callback must
  use DRAM-safe macros (ESP_DRAM_LOGI). ESP_ERROR_CHECK must not be used inside the callback
  as it calls abort(), which is unsafe from ISR context.
- All ESP-IDF calls outside the callback use ESP_ERROR_CHECK.
- The XIAO ESP32S3 uses native USB CDC for serial; CONFIG_ESP_CONSOLE_USB_CDC=y is required
  in sdkconfig.defaults when XIAO S3 is the active board.
- The XIAO ESP32-C5 uses the USB Serial/JTAG controller; CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y
  is required in sdkconfig.defaults when XIAO C5 is the active board.
- The ESP32-C6-DevKitC-1-N8 uses the left USB-C port (UART bridge, UART0); the default UART
  console is correct — do NOT add CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y for this board.

## Gotchas

- LEDC_HIGH_SPEED_MODE is unavailable on ESP32-S3, ESP32-C5, and ESP32-C6; accidentally using
  it causes a link-time error, not a compile-time warning. The `#ifdef CONFIG_IDF_TARGET_ESP32`
  guard is the correct resolution — not a Kconfig board choice.
- The HUZZAH32 LED is active HIGH; XIAO ESP32S3, XIAO ESP32-C5, and XIAO ESP32-C6 LEDs are
  active LOW. Failing to invert duty correctly results in the LED being off when it should be on.
- The XIAO ESP32-C5 console config is CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y, not
  CONFIG_ESP_CONSOLE_USB_CDC=y. These are different peripherals; using the wrong one
  produces no monitor output.
- For WS2812 boards: LEDC_ACTIVE_LEVEL and GPIO polarity are irrelevant — WS2812 is always
  driven by RMT data frames, never by duty cycle.
- `idf.py set-target <chip>` is the only step needed when switching to a listed board — the
  per-target `sdkconfig.defaults.<target>` file automatically injects the correct LED GPIO,
  type, polarity, and console config. No menuconfig step is required.

## File Layout (non-standard files)

- main/Kconfig.projbuild — defines EXAMPLE_LED_GPIO, EXAMPLE_LED_ACTIVE_LEVEL, EXAMPLE_LED_WS2812
- sdkconfig.defaults.<target> — per-target overrides for GPIO, LED type, polarity, console config
