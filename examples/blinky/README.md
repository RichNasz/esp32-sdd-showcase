<!-- ================================================
     AGENT-GENERATED — DO NOT EDIT BY HAND
     Generated from specs/ using esp32-sdd-full-project-generator skill
     Date: 2026-04-12 | Agent: Claude Code
     ================================================ -->

# Breathing LED — Multi-Board Demo (LEDC + WS2812)

> **SDD baseline.** Demonstrates the full spec-to-firmware pipeline. Drives a smooth breathing
> effect on the onboard status LED using either the ESP-IDF LEDC driver (hardware PWM, for simple
> GPIO LEDs) or the RMT peripheral (800 kHz NRZ protocol, for WS2812 RGB LEDs), selected at
> compile time via a Kconfig bool. Covers four boards across three manufacturers.

## Overview

Drives a smooth, continuous breathing effect on the onboard status LED of whichever board is
selected. Breathing period is 4 seconds (2 s fade up, 2 s fade down).

Two code paths are selected at compile time via the `EXAMPLE_LED_WS2812` Kconfig bool:

- **LEDC path** (simple GPIO LED boards): entirely interrupt-driven via the LEDC fade-done
  callback — no busy-wait loops, no polling tasks.
- **WS2812 path** (DevKitC with addressable RGB LED): RMT peripheral drives the 800 kHz NRZ
  protocol; brightness cycles white (R=G=B) from 0 → 64 → 0 in a software loop.

Board selection is handled by `idf.py set-target` alone. Per-target `sdkconfig.defaults.<target>`
files automatically inject the correct LED GPIO, LED type, and console config — no `menuconfig`
step required for the primary boards listed below.

## Prerequisites

- ESP-IDF v5.5.x (tested on v5.5.3)
- One of the supported boards listed below
- USB cable appropriate for your board

## Supported Boards

| `idf.py set-target` | Board | LED GPIO | LED Type | Console |
|---|---|---|---|---|
| `esp32` | Adafruit HUZZAH32 | GPIO 13 | Simple GPIO, active HIGH | UART (CP2104) |
| `esp32s3` | Seeed XIAO ESP32S3 | GPIO 21 | Simple GPIO, active LOW | USB CDC |
| `esp32c5` | Seeed XIAO ESP32-C5 | GPIO 27 | Simple GPIO, active LOW | USB Serial/JTAG |
| `esp32c6` | Espressif ESP32-C6-DevKitC-1-N8 | GPIO 8 | WS2812 RGB | UART bridge (left USB-C) |

## Build & Flash

```sh
cd examples/blinky

# Always clear any stale sdkconfig before switching boards or after updating defaults:
rm -f sdkconfig sdkconfig.old

# Select your board — the per-target sdkconfig.defaults.<target> handles everything else:
idf.py set-target esp32c6   # or esp32, esp32s3, esp32c5

idf.py build
idf.py flash -p /dev/cu.usbserial-*   # adjust port to match your system
```

Monitor in a separate terminal (`idf.py monitor` requires an interactive TTY):

```sh
idf.py monitor -p /dev/cu.usbserial-*
```

> **Never edit generated source files directly.** To change board configuration, update
> `specs/FunctionalSpec.md` or `specs/CodingSpec.md` and regenerate.

## Opening in VS Code / Cursor

This folder is a complete, standalone ESP-IDF project. Open **only this folder**
(not the repository root) in VS Code or Cursor so the official ESP-IDF Extension
can detect and configure the project automatically.

**File → Open Folder… → select `examples/blinky/`**

This gives you one-click Build / Flash / Monitor / Debug, Menuconfig, and size
analysis from the ESP-IDF commands palette. Do not open the top-level repository
folder as your active workspace while developing this example.

## Multi-Manufacturer Same-Chip Handling

Two boards from different manufacturers can share the same ESP32 chip variant and therefore
the same IDF target name (e.g. `esp32c6`). Because only one `sdkconfig.defaults.esp32c6`
can exist, one board is designated **primary** and its settings are hardcoded in that file.
Other boards of the same chip are **secondary** and need one extra step.

### Primary boards (zero extra steps — `set-target` does everything)

| Target | Primary board | Why it's primary |
|---|---|---|
| `esp32` | Adafruit HUZZAH32 | Only esp32 board in this example |
| `esp32s3` | Seeed XIAO ESP32S3 | Only esp32s3 board in this example |
| `esp32c5` | Seeed XIAO ESP32-C5 | Only esp32c5 board in this example |
| `esp32c6` | Espressif ESP32-C6-DevKitC-1-N8 | WS2812 LED requires a distinct code path; documented as primary |

### Secondary boards (one menuconfig step required)

**Seeed XIAO ESP32-C6** — same `esp32c6` chip, simple GPIO LED on GPIO 15 (active LOW):

```sh
rm -f sdkconfig sdkconfig.old
idf.py set-target esp32c6
idf.py menuconfig
# Navigate to: Blinky Configuration
#   "LED is a WS2812B..." → disabled   (EXAMPLE_LED_WS2812=n)
#   "LED GPIO number"     → 15          (EXAMPLE_LED_GPIO=15)
# Save and exit
idf.py build && idf.py flash
```

### General rule for adding a new board of an existing chip

1. **Identify the differences** — LED type, GPIO, console peripheral (UART vs USB CDC vs USB Serial/JTAG).
2. **Express each difference as a Kconfig symbol** — `bool` for type switches, `int` for GPIO and polarity. Never add board-specific `#ifdef` in C source; all differences live in Kconfig.
3. **Decide which board is primary** — update `sdkconfig.defaults.<target>` for it.
4. **Document secondary boards** — list them in `specs/FunctionalSpec.md` with their menuconfig overrides. The README is regenerated from that spec.
5. **`rm -f sdkconfig sdkconfig.old` before every board switch** — `idf.py set-target` re-reads `sdkconfig.defaults.*` from scratch, but a stale `sdkconfig` on disk takes precedence and silently shadows the defaults.

> **Symptom of a stale sdkconfig:** bootloader output appears on the monitor but the application
> is silent. This usually means the old sdkconfig still has `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y`
> routing application output to the native USB JTAG port while you're monitoring the UART bridge.
> Fix: `rm -f sdkconfig sdkconfig.old && idf.py set-target <chip> && idf.py build && idf.py flash`.

## Expected Serial Output

### ESP32-C6-DevKitC-1-N8 (WS2812 RGB, GPIO 8)

```
I (317) blinky: Blinky — WS2812 RGB LED, GPIO 8, breathing period 4000 ms
I (318) blinky: WS2812 RMT channel init on GPIO 8 (10 MHz, GRB order, max brightness 64/255)
I (320) blinky: Starting WS2812 breathing loop
```

The RGB LED breathes white (dim → bright → dim) continuously.

### Adafruit HUZZAH32 (active HIGH, GPIO 13)

```
I (320) blinky: Board GPIO=13  freq=5000 Hz  resolution=13-bit  polarity=active-HIGH
I (322) blinky: Breathing period: 4000 ms
I (324) blinky: Fade up → duty 8191
I (2326) blinky: Fade down → duty 0
I (4328) blinky: Fade up → duty 8191
```

### Seeed XIAO ESP32S3 (active LOW, GPIO 21)

```
I (320) blinky: Board GPIO=21  freq=5000 Hz  resolution=13-bit  polarity=active-LOW
I (322) blinky: Breathing period: 4000 ms
I (324) blinky: Fade up → duty 0
I (2326) blinky: Fade down → duty 8191
I (4328) blinky: Fade up → duty 0
```

## Key Concepts

- **WS2812 via RMT**: `rmt_new_tx_channel` + `rmt_new_bytes_encoder` drives the 800 kHz NRZ
  protocol; GRB byte order; 10 MHz RMT resolution (1 tick = 100 ns); brightness capped at
  64/255 to protect eyes from bare-LED intensity
- **LEDC fade callback**: `ledc_fade_func_install()` + IRAM-resident fade-done ISR — interrupt-
  driven fading eliminates CPU involvement during transitions
- **`EXAMPLE_LED_WS2812` Kconfig bool**: compile-time code path selector — all board differences
  stay in `sdkconfig.defaults.<target>`, never in C source
- **`#ifdef CONFIG_IDF_TARGET_ESP32`** selects `LEDC_HIGH_SPEED_MODE` vs `LEDC_LOW_SPEED_MODE` —
  a genuine SoC architectural difference, not a board choice
- **Active-LOW polarity inversion**: XIAO boards require `DUTY_ON=0`, `DUTY_OFF=DUTY_MAX` — driven
  by `EXAMPLE_LED_ACTIVE_LEVEL`; irrelevant for the WS2812 path
- **`ESP_DRAM_LOGI`** inside the IRAM-resident LEDC fade callback — `ESP_LOGI` is not flash-safe
  from ISR context
- **`rm -f sdkconfig sdkconfig.old` discipline**: required before every board switch to force
  `sdkconfig.defaults.*` to be re-applied from scratch

## Testing

Testing philosophy: automated first, manual only when hardware observation is unavoidable.

### Automated Tests

1. **T-A1 — Zero-Warning Build (HUZZAH32)**

   ```sh
   cd examples/blinky && rm -f sdkconfig sdkconfig.old
   idf.py set-target esp32
   idf.py build 2>&1 | grep -E "warning:|error:|ninja: build stopped"
   ```

   Pass: exit code 0 from `idf.py build`, grep produces no output.

2. **T-A2 — Zero-Warning Build (XIAO ESP32S3)**

   ```sh
   rm -f sdkconfig sdkconfig.old && idf.py set-target esp32s3
   idf.py build 2>&1 | grep -E "warning:|error:|ninja: build stopped"
   ```

   Pass: exit code 0 from `idf.py build`, grep produces no output.

3. **T-A3 — Zero-Warning Build (ESP32-C6-DevKitC — WS2812 path)**

   ```sh
   rm -f sdkconfig sdkconfig.old && idf.py set-target esp32c6
   idf.py build 2>&1 | grep -E "warning:|error:|ninja: build stopped"
   ```

   Pass: exit code 0 from `idf.py build`, grep produces no output.

4. **T-A4 — sdkconfig Verification (DevKitC)**

   Run immediately after T-A3 (reuses its sdkconfig). Guards against a stale sdkconfig
   shadowing the updated `sdkconfig.defaults.esp32c6`.

   ```sh
   grep -E "EXAMPLE_LED_WS2812|EXAMPLE_LED_GPIO|ESP_CONSOLE_USB_SERIAL_JTAG" sdkconfig
   ```

   Pass: output contains `CONFIG_EXAMPLE_LED_WS2812=y`, `CONFIG_EXAMPLE_LED_GPIO=8`, and
   `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG` is **not set**.

5. **T-A5 — Binary Size Sanity Check**

   ```sh
   idf.py size | grep "Total binary size"
   ```

   Pass: reported binary size < 1 MB.

### Manual Tests — hardware required

**T-M1 — Visual LED Breathing Verification (HUZZAH32)**

Why manual: smooth optical fade quality requires human perception; a build check confirms
register writes but cannot certify perceptual smoothness.

Hardware required: Adafruit HUZZAH32, USB Micro-B cable.

1. `rm -f sdkconfig sdkconfig.old && idf.py set-target esp32 && idf.py build flash`
2. Observe the red LED on the HUZZAH32 (GPIO 13).
3. Confirm LED fades from off → fully on → off smoothly over ~4 seconds per cycle.
4. Confirm no visible steps or flicker during the fade.
5. Confirm cycle repeats indefinitely without stopping.

Pass: smooth, continuous breathing with no visible artefacts.

---

**T-M2 — Visual LED Breathing Verification (XIAO ESP32S3)**

Why manual: same as T-M1; additionally the active-LOW polarity inversion requires confirmation
on real silicon.

Hardware required: Seeed XIAO ESP32S3, USB-C cable.

1. `rm -f sdkconfig sdkconfig.old && idf.py set-target esp32s3 && idf.py build flash`
2. Observe the amber LED on the XIAO (GPIO 21).
3. Confirm LED starts dark, then brightens (first fade is dark → bright per spec).
4. Confirm smooth, continuous breathing over ~4 seconds per cycle.
5. Confirm serial output shows `Fade up` on first fade, then alternating `Fade up` / `Fade down`.

Pass: correct polarity, smooth breathing, correct serial log sequence.

---

**T-M3 — Visual WS2812 Breathing Verification (ESP32-C6-DevKitC-1-N8)**

Why manual: WS2812 breathing is a software RMT loop — protocol errors produce no light or
solid-on, which build checks cannot detect.

Hardware required: Espressif ESP32-C6-DevKitC-1-N8, USB-C cable to **left port** (UART bridge,
`/dev/cu.usbserial-*` on macOS).

1. `rm -f sdkconfig sdkconfig.old && idf.py set-target esp32c6 && idf.py build`
2. `idf.py flash -p /dev/cu.usbserial-*`
3. In a terminal: `idf.py monitor -p /dev/cu.usbserial-*`
4. Confirm serial output: `WS2812 RMT channel init on GPIO 8` then `Starting WS2812 breathing loop`.
5. Confirm the RGB LED breathes white (dim → bright → dim) with no stuck-on or stuck-off condition.
6. Confirm breathing period is approximately 4 seconds per cycle.

Pass: LED breathes white smoothly; serial log confirms RMT init and loop start.

---

**T-M4 — Secondary Board Override (Seeed XIAO ESP32-C6)**

Why manual: verifies the menuconfig override path for secondary boards sharing a chip target.
Cannot be automated without a two-board CI rig that also validates visual LED behaviour.

Hardware required: Seeed XIAO ESP32-C6, USB-C cable.

1. `rm -f sdkconfig sdkconfig.old && idf.py set-target esp32c6`
2. `idf.py menuconfig` → **Blinky Configuration** → set `LED is a WS2812B...` to disabled,
   `LED GPIO number` to 15. Save and exit.
3. `idf.py build && idf.py flash`
4. Confirm serial output shows LEDC init (`freq=5000 Hz`), not WS2812 init.
5. Confirm the LED on GPIO 15 breathes smoothly.

Pass: LEDC breathing on GPIO 15; no `WS2812 RMT` in serial log.

---

**T-M5 — Board Switch Regression**

Why manual: requires physically reflashing two different boards and observing both.

1. Flash HUZZAH32 (esp32 target) — confirm T-M1 passes.
2. `rm -f sdkconfig sdkconfig.old && idf.py set-target esp32s3 && idf.py build flash`
3. Confirm T-M2 passes on the XIAO ESP32S3.
4. Switch back to HUZZAH32 and repeat — confirm T-M1 still passes.

Pass: both boards pass their visual checks after each switch.

## Spec Files

- [specs/FunctionalSpec.md](specs/FunctionalSpec.md)
- [specs/CodingSpec.md](specs/CodingSpec.md)
- [specs/TestSpec.md](specs/TestSpec.md)

## Board Specs

- [board-specs/adafruit/huzzah32.md](../../board-specs/adafruit/huzzah32.md)
- [board-specs/seeed/xiao-esp32s3.md](../../board-specs/seeed/xiao-esp32s3.md)
- [board-specs/seeed/xiao-esp32c5.md](../../board-specs/seeed/xiao-esp32c5.md)
- [board-specs/espressif/esp32-c6-devkitc-1-n8.md](../../board-specs/espressif/esp32-c6-devkitc-1-n8.md)
