<!-- ================================================
     AGENT-GENERATED — DO NOT EDIT BY HAND
     Generated from specs/ using esp32-sdd-full-project-generator skill
     Date: 2026-02-22 | Agent: Claude Code
     ================================================ -->

# 01 · Blinky — PWM Breathing LED (Multi-Board)

> **Baseline example.** Demonstrates the full spec-to-firmware SDD pipeline using
> the ESP-IDF LEDC driver for a smooth breathing effect. Supports two boards via
> Kconfig-driven GPIO selection — no hard-coded pin numbers.

## Prerequisites

- **ESP-IDF 5.x** installed and `idf.py` on your PATH
- One of the supported boards (see table below)
- USB cable for flashing and serial monitoring

## Supported Boards

| Board | SoC | LED GPIO | Polarity | idf.py target |
|---|---|---|---|---|
| Adafruit HUZZAH32 (ESP32 Feather) | ESP32 | GPIO 13 | Active HIGH | `esp32` |
| Seeed XIAO ESP32S3 | ESP32-S3 | GPIO 21 | Active LOW | `esp32s3` |

> **Active LOW** means the LED turns on when the GPIO is driven LOW (duty = 0).
> The firmware handles this automatically via `LED_ACTIVE_LOW` / `DUTY_ON` / `DUTY_OFF` macros.

## What It Demonstrates

- SDD project layout: `FunctionalSpec.md` → `CodingSpec.md` → fully generated project
- ESP-IDF `ledc` driver for hardware PWM output (no busy-wait loops)
- Smooth breathing effect via LEDC fade interrupt callbacks
- Kconfig board abstraction — `CONFIG_BOARD_*` selects GPIO, LEDC speed mode, and LED polarity at compile time
- LEDC speed mode differences: `LEDC_HIGH_SPEED_MODE` (ESP32) vs `LEDC_LOW_SPEED_MODE` (ESP32-S3)
- Active LED polarity abstraction — `DUTY_ON`/`DUTY_OFF` macros produce correct duty values for both active-HIGH and active-LOW LEDs

## Opening in VS Code / Cursor

```sh
code examples/01-blinky
# or
cursor examples/01-blinky
```

Install the **ESP-IDF extension** (Espressif IDF) for IntelliSense, build, and flash support directly from the editor. After opening, select the correct COM port and target in the extension's status bar.

## Build & Flash

### Adafruit HUZZAH32 (default)

```sh
cd examples/01-blinky
idf.py set-target esp32
idf.py build flash monitor
```

### Seeed XIAO ESP32S3

```sh
cd examples/01-blinky
idf.py set-target esp32s3
idf.py build flash monitor
```

> **XIAO serial output note:** The XIAO ESP32S3 uses native USB (no USB-to-UART chip).
> If `idf.py monitor` shows no output, add `CONFIG_ESP_CONSOLE_USB_CDC=y` to
> `sdkconfig.defaults` (see `specs/CodingSpec.md` — USB-CDC section).

## Switching Boards

1. Open `specs/CodingSpec.md` and change the `# Board:` comment at the top.
2. Update `sdkconfig.defaults` to set the matching `CONFIG_BOARD_*=y` line.
3. Run `idf.py set-target <target>` for the new chip.
4. Run `idf.py build`.

## Expected Serial Output

### HUZZAH32 (active HIGH, GPIO 13)

```
I (320) blinky: Board GPIO=13  freq=5000 Hz  resolution=8192-bit  polarity=active-HIGH
I (322) blinky: Fade started — breathing period: 4000 ms
```

### XIAO ESP32S3 (active LOW, GPIO 21)

```
I (320) blinky: Board GPIO=21  freq=5000 Hz  resolution=8192-bit  polarity=active-LOW
I (322) blinky: Fade started — breathing period: 4000 ms
```

The LED breathes smoothly through a full 4-second cycle (2 s up, 2 s down), indefinitely.

## Key Concepts

- `ledc_timer_config_t` / `ledc_channel_config_t`
- `ledc_fade_func_install()` + `ledc_set_fade_with_time()`
- `ledc_cb_register()` — interrupt-driven fade completion callback
- Kconfig `choice` block for multi-board selection
- `CONFIG_BOARD_*` macros for compile-time GPIO and mode resolution
- `LED_ACTIVE_LOW` / `DUTY_ON` / `DUTY_OFF` — active LED polarity abstraction

## Regeneration Note

This example is fully agent-generated from its spec files. If anything looks wrong, improve the spec and regenerate — never edit generated files by hand. See [`shared-specs/AIGenLessonsLearned.md`](../../shared-specs/AIGenLessonsLearned.md) for the project's gold standard rules and lessons learned.

## Spec Files

- [FunctionalSpec.md](specs/FunctionalSpec.md)
- [CodingSpec.md](specs/CodingSpec.md)

## Board Specs

- [board-specs/adafruit/huzzah32.md](../../board-specs/adafruit/huzzah32.md)
- [board-specs/seeed/xiao-esp32s3.md](../../board-specs/seeed/xiao-esp32s3.md)
