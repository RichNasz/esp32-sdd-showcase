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

## Testing

Testing philosophy: automated first, manual only when hardware observation is unavoidable.

### Automated Tests

1. **Zero-warning build — HUZZAH32 (T-A1)**

   ```sh
   cd examples/blinky
   idf.py set-target esp32 && idf.py build
   ```

   Pass: exit code 0, zero compiler warnings.

2. **Zero-warning build — XIAO ESP32S3 (T-A2)**

   ```sh
   idf.py set-target esp32s3 && idf.py build
   ```

   Pass: exit code 0, zero compiler warnings.

3. **Binary size check (T-A3)**

   ```sh
   idf.py size | grep "Total binary size"
   ```

   Pass: reported size < 1 MB.

4. **Wokwi simulation — HUZZAH32 (T-A4)**

   Requires Wokwi CLI (`curl -L https://wokwi.com/ci/install.sh | sh`) and a `wokwi.toml` + `diagram.json` at the example root.

   ```sh
   wokwi-cli --timeout 10000 --expect "Breathing period" .
   ```

   Pass: `Breathing period` appears in simulated serial output within 10 seconds.

### Manual — hardware required

*The following steps require physical hardware because smooth optical fade quality and active-LOW polarity inversion cannot be fully verified by simulation.*

1. **LED breathing — HUZZAH32 (T-M1)**
   Flash with `idf.py set-target esp32 && idf.py build flash`.
   Observe the red LED (GPIO 13): confirm smooth fade from off → fully on → off over ~4 seconds, repeating continuously with no visible steps or flicker.

2. **LED breathing — XIAO ESP32S3 (T-M2)**
   Flash with `idf.py set-target esp32s3 && idf.py build flash -p /dev/cu.usbmodem*`.
   Observe the amber LED (GPIO 21): confirm the first fade is dark → bright (active-LOW polarity correct), then smooth continuous breathing. Check serial output shows `Fade up` on first cycle, then alternating `Fade up` / `Fade down`.

3. **Board switch regression (T-M3)**
   Repeat T-M1 and T-M2 after switching the active board in `specs/CodingSpec.md` and regenerating. Both boards must pass on every switch.

Full test details: [specs/TestSpec.md](specs/TestSpec.md)

## Spec Files

- [FunctionalSpec.md](specs/FunctionalSpec.md)
- [CodingSpec.md](specs/CodingSpec.md)
- [TestSpec.md](specs/TestSpec.md)

## Board Specs

- [board-specs/adafruit/huzzah32.md](../../board-specs/adafruit/huzzah32.md)
- [board-specs/seeed/xiao-esp32s3.md](../../board-specs/seeed/xiao-esp32s3.md)
