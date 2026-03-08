<!-- ================================================
     AGENT-GENERATED — DO NOT EDIT BY HAND
     Generated from specs/ using esp32-sdd-full-project-generator skill
     Date: 2026-03-08 | Agent: Claude Code
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

| Board | SoC | LED GPIO | Polarity | Console config | `idf.py` target |
| --- | --- | --- | --- | --- | --- |
| Seeed XIAO ESP32S3 *(default)* | ESP32-S3 | GPIO 21 | Active LOW | `CONFIG_ESP_CONSOLE_USB_CDC=y` | `esp32s3` |
| Adafruit HUZZAH32 (ESP32 Feather) | ESP32 | GPIO 13 | Active HIGH | *(UART — no extra config)* | `esp32` |
| Seeed XIAO ESP32-C5 | ESP32-C5 | GPIO 27 | Active LOW | `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y` | `esp32c5` |
| Seeed XIAO ESP32-C6 | ESP32-C6 | GPIO 15 | Active LOW | `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y` | `esp32c6` |

- Matching USB cable for flashing and serial monitoring

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

### Seeed XIAO ESP32-C5

```sh
cd examples/blinky
idf.py set-target esp32c5
idf.py build flash monitor
```

### Seeed XIAO ESP32-C6

```sh
cd examples/blinky
idf.py set-target esp32c6
idf.py build flash monitor
```

> **Switching boards — two required steps, in order:**
>
> 1. `idf.py set-target <chip>` — e.g. `idf.py set-target esp32c6`. This clears the
>    stale `sdkconfig`; skipping this step leaves the old board's GPIO in effect and
>    the LED will not respond.
> 2. `idf.py menuconfig` → **Target board** → select the new board → Save & Exit.
>
> Then rebuild and reflash. Never edit generated source files directly.

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

```text
I (320) blinky: Board GPIO=21  freq=5000 Hz  resolution=13-bit  polarity=active-LOW
I (322) blinky: Breathing period: 4000 ms
I (324) blinky: Fade up → duty 0
I (2326) blinky: Fade down → duty 8191
I (4328) blinky: Fade up → duty 0
```

### Adafruit HUZZAH32 (active HIGH, GPIO 13)

```text
I (320) blinky: Board GPIO=13  freq=5000 Hz  resolution=13-bit  polarity=active-HIGH
I (322) blinky: Breathing period: 4000 ms
I (324) blinky: Fade up → duty 8191
I (2326) blinky: Fade down → duty 0
I (4328) blinky: Fade up → duty 8191
```

### XIAO ESP32-C5 (active LOW, GPIO 27)

```text
I (320) blinky: Board GPIO=27  freq=5000 Hz  resolution=13-bit  polarity=active-LOW
I (322) blinky: Breathing period: 4000 ms
I (324) blinky: Fade up → duty 0
I (2326) blinky: Fade down → duty 8191
I (4328) blinky: Fade up → duty 0
```

### XIAO ESP32-C6 (active LOW, GPIO 15)

```text
I (320) blinky: Board GPIO=15  freq=5000 Hz  resolution=13-bit  polarity=active-LOW
I (322) blinky: Breathing period: 4000 ms
I (324) blinky: Fade up → duty 0
I (2326) blinky: Fade down → duty 8191
I (4328) blinky: Fade up → duty 0
```

The LED breathes smoothly through a full 4-second cycle (2 s up, 2 s down), indefinitely.

## Key Concepts

- `ledc_timer_config_t` / `ledc_channel_config_t` — hardware PWM timer and channel setup
- `ledc_fade_func_install()` + `ledc_set_fade_with_time()` — smooth fade without CPU polling
- `ledc_cb_register()` — interrupt-driven fade completion callback chaining the next fade
- `LEDC_HIGH_SPEED_MODE` (ESP32) vs `LEDC_LOW_SPEED_MODE` (ESP32-S3, ESP32-C5) — speed mode differences per SoC family
- Kconfig `choice` block in `main/Kconfig.projbuild` for multi-board selection
- `CONFIG_BOARD_*` macros for compile-time GPIO and LEDC mode resolution
- `LED_ACTIVE_LOW` / `DUTY_ON` / `DUTY_OFF` — active LED polarity abstraction
- `CONFIG_ESP_CONSOLE_USB_CDC=y` — required for serial output on XIAO ESP32S3 (native USB OTG)
- `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y` — required for serial output on XIAO ESP32-C5 (USB Serial/JTAG controller, distinct from S3's USB OTG)

## Testing

Testing philosophy: automated first, manual only when hardware observation is unavoidable.

### Automated Tests

1. **Zero-warning build — HUZZAH32 (T-A1)**

   ```sh
   cd examples/blinky
   idf.py set-target esp32
   idf.py build 2>&1 | grep -E "warning:|error:|ninja: build stopped"
   ```

   Pass: exit code 0, grep produces no output.

2. **Zero-warning build — XIAO ESP32S3 (T-A2)**

   ```sh
   idf.py set-target esp32s3
   idf.py build 2>&1 | grep -E "warning:|error:|ninja: build stopped"
   ```

   Pass: exit code 0, grep produces no output.

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
   Repeat T-M1 and T-M2 after switching boards via `idf.py menuconfig`. Both boards must pass on every switch.

Full test details: [specs/TestSpec.md](specs/TestSpec.md)

## Spec Files

- [FunctionalSpec.md](specs/FunctionalSpec.md)
- [CodingSpec.md](specs/CodingSpec.md)
- [TestSpec.md](specs/TestSpec.md)

## Board Specs

- [board-specs/adafruit/huzzah32.md](../../board-specs/adafruit/huzzah32.md)
- [board-specs/seeed/xiao-esp32s3.md](../../board-specs/seeed/xiao-esp32s3.md)
- [board-specs/seeed/xiao-esp32c5.md](../../board-specs/seeed/xiao-esp32c5.md)
- [board-specs/seeed/xiao-esp32c6.md](../../board-specs/seeed/xiao-esp32c6.md)
