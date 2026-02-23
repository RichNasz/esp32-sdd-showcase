<!-- ================================================
     AGENT-GENERATED — DO NOT EDIT BY HAND
     Generated from specs/ using esp32-sdd-full-project-generator skill
     Date: 2026-02-22 | Agent: Claude Code
     ================================================ -->

# Blinky — PWM Breathing LED (Multi-Board)

## Overview

Baseline SDD example demonstrating the full spec-to-firmware pipeline. The ESP-IDF LEDC
driver drives a smooth 4-second breathing effect on the onboard status LED of whichever
board is selected. Board selection is a Kconfig option — GPIO number, LEDC speed mode, and
LED polarity are all resolved at compile time via `CONFIG_BOARD_*` macros. No hard-coded
pin numbers, no busy-wait loops.

## Prerequisites

- **ESP-IDF 5.x** installed and `idf.py` on your PATH
- One of the supported boards:

| Board | SoC | LED GPIO | Polarity | idf.py target |
|---|---|---|---|---|
| Seeed XIAO ESP32S3 *(default)* | ESP32-S3 | GPIO 21 | Active LOW | `esp32s3` |
| Adafruit HUZZAH32 (ESP32 Feather) | ESP32 | GPIO 13 | Active HIGH | `esp32` |

- USB-C cable (XIAO) or USB Micro-B cable (HUZZAH32) for flashing and serial monitoring

## Build & Flash

### Seeed XIAO ESP32S3 (default)

```sh
cd examples/blinky
idf.py set-target esp32s3
idf.py build flash monitor
```

### Adafruit HUZZAH32

```sh
cd examples/blinky
idf.py set-target esp32
idf.py build flash monitor
```

> **Switching boards:** Open `specs/CodingSpec.md`, update the `BOARD SELECTION` block at the
> top, then regenerate via the `esp32-sdd-full-project-generator` skill. Never edit
> generated files by hand.

### Opening in VS Code / Cursor

This folder is a complete, standalone ESP-IDF project. Open **only this folder**
(not the repository root) in VS Code or Cursor so the official ESP-IDF Extension
can detect and configure the project automatically.

**File → Open Folder… → select `examples/blinky/`**

This gives you one-click Build / Flash / Monitor / Debug, Menuconfig, and size
analysis from the ESP-IDF commands palette. Do not open the top-level repository
folder as your active workspace while developing this example.

## Expected Serial Output

### XIAO ESP32S3 (active LOW, GPIO 21)

```
I (320) blinky: Board GPIO=21  freq=5000 Hz  resolution=8192-bit  polarity=active-LOW
I (322) blinky: Fade started — breathing period: 4000 ms
```

### HUZZAH32 (active HIGH, GPIO 13)

```
I (320) blinky: Board GPIO=13  freq=5000 Hz  resolution=8192-bit  polarity=active-HIGH
I (322) blinky: Fade started — breathing period: 4000 ms
```

The LED breathes smoothly through a full 4-second cycle (2 s up, 2 s down), indefinitely.

## Key Concepts

- `ledc_timer_config_t` / `ledc_channel_config_t` — hardware PWM timer and channel setup
- `ledc_fade_func_install()` + `ledc_set_fade_with_time()` — smooth fade without CPU polling
- `ledc_cb_register()` — interrupt-driven fade completion callback chaining the next fade
- `LEDC_HIGH_SPEED_MODE` (ESP32) vs `LEDC_LOW_SPEED_MODE` (ESP32-S3) — speed mode differences
- Kconfig `choice` block in `main/Kconfig.projbuild` for multi-board selection
- `CONFIG_BOARD_*` macros for compile-time GPIO and LEDC mode resolution
- `LED_ACTIVE_LOW` / `DUTY_ON` / `DUTY_OFF` — active LED polarity abstraction
- `CONFIG_ESP_CONSOLE_USB_CDC=y` — required for serial output on XIAO (native USB, no UART bridge)

## Spec Files

- [FunctionalSpec.md](specs/FunctionalSpec.md)
- [CodingSpec.md](specs/CodingSpec.md)

## Board Specs

- [board-specs/adafruit/huzzah32.md](../../board-specs/adafruit/huzzah32.md)
- [board-specs/seeed/xiao-esp32s3.md](../../board-specs/seeed/xiao-esp32s3.md)
