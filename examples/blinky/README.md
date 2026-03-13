<!-- ================================================
     AGENT-GENERATED — DO NOT EDIT BY HAND
     Generated from specs/ using esp32-sdd-documentation-generator skill
     Date: 2026-03-13 | Agent: Claude Code
     ================================================ -->

# Breathing LED — Multi-Board PWM Demo

> **SDD baseline.** Demonstrates the full spec-to-firmware pipeline. Hardware PWM breathing effect, four boards, zero external components.

## Overview

Uses the ESP-IDF LEDC driver to drive a smooth, continuous breathing effect on the onboard
status LED. Breathing period is 4 seconds (2 s fade up, 2 s fade down). The effect is entirely
interrupt-driven via the LEDC fade-done callback — no busy-wait loops, no polling tasks.

Board selection is handled by `idf.py set-target` alone. Per-target `sdkconfig.defaults.<target>`
files automatically inject the correct LED GPIO, polarity, and console config — no `menuconfig`
step required.

## Supported Boards

| `idf.py set-target` | Board | LED GPIO | LED Polarity | Console |
|---|---|---|---|---|
| `esp32` | Adafruit HUZZAH32 | GPIO 13 | Active HIGH | UART (CP2104) |
| `esp32s3` | Seeed XIAO ESP32S3 | GPIO 21 | Active LOW | USB CDC |
| `esp32c5` | Seeed XIAO ESP32-C5 | GPIO 27 | Active LOW | USB Serial/JTAG |
| `esp32c6` | Seeed XIAO ESP32-C6 | GPIO 15 | Active LOW | USB Serial/JTAG |

No external components required on any board.

## Build & Flash

```sh
cd examples/blinky

# Select your board — the per-target sdkconfig.defaults.<target> handles everything else:
idf.py set-target esp32s3   # or esp32, esp32c5, esp32c6

idf.py build flash monitor
```

> **Switching boards**: `idf.py set-target <chip>` is the only step needed. The per-target
> `sdkconfig.defaults.<target>` file automatically injects the correct LED GPIO, polarity,
> and console config. Never edit generated source files directly.

## Expected Serial Output

### XIAO ESP32S3 (active LOW, GPIO 21)

```
I (320) blinky: Board GPIO=21  freq=5000 Hz  resolution=13-bit  polarity=active-LOW
I (322) blinky: Breathing period: 4000 ms
I (324) blinky: Fade up → duty 0
I (2326) blinky: Fade down → duty 8191
I (4328) blinky: Fade up → duty 0
```

### Adafruit HUZZAH32 (active HIGH, GPIO 13)

```
I (320) blinky: Board GPIO=13  freq=5000 Hz  resolution=13-bit  polarity=active-HIGH
I (322) blinky: Breathing period: 4000 ms
I (324) blinky: Fade up → duty 8191
I (2326) blinky: Fade down → duty 0
I (4328) blinky: Fade up → duty 8191
```

## Key Concepts

- `ledc_fade_func_install()` + fade-done ISR callback — interrupt-driven fading eliminates CPU
  involvement during the fade transition
- `LEDC_HIGH_SPEED_MODE` on ESP32; `LEDC_LOW_SPEED_MODE` on all other targets — selected via
  `#ifdef CONFIG_IDF_TARGET_ESP32` (a genuine SoC architectural difference, not a board choice)
- Active-LOW polarity on XIAO boards requires duty-cycle inversion: full brightness = low duty,
  off = high duty
- Per-target `sdkconfig.defaults.<target>` injects `EXAMPLE_LED_GPIO` and
  `EXAMPLE_LED_ACTIVE_LEVEL` as Kconfig int symbols; no menuconfig step needed when switching
- `ESP_DRAM_LOGI` inside the IRAM-resident fade callback (ESP_LOGI is not flash-safe from ISR)

## Testing

Testing philosophy: automated first, manual only when hardware observation is unavoidable.

### Automated Tests

1. **T-A1 — Zero-Warning Build (HUZZAH32)**

   ```sh
   cd examples/blinky
   idf.py set-target esp32
   idf.py build 2>&1 | grep -E "warning:|error:|ninja: build stopped"
   ```

   Pass: exit code 0 from `idf.py build`, grep produces no output.

2. **T-A2 — Zero-Warning Build (XIAO ESP32S3)**

   ```sh
   idf.py set-target esp32s3
   idf.py build 2>&1 | grep -E "warning:|error:|ninja: build stopped"
   ```

   Pass: exit code 0 from `idf.py build`, grep produces no output.

3. **T-A3 — Binary Size Sanity Check**

   ```sh
   idf.py size | grep "Total binary size"
   ```

   Pass: reported binary size < 1 MB.

4. **T-A4 — Wokwi Simulation (HUZZAH32)**

   Justification: provides hardware-level LEDC simulation without physical hardware; freely
   available, single binary install, no account required for local use.

   Prerequisites: [Install Wokwi CLI](https://docs.wokwi.com/wokwi-ci/getting-started);
   `wokwi.toml` + `diagram.json` at the example root targeting the HUZZAH32.

   ```sh
   wokwi-cli --timeout 10000 --expect "Breathing period" .
   ```

   Pass: `Breathing period` appears in simulated serial output within 10 seconds.

### Manual Tests — hardware required

**T-M1 — Visual LED Breathing Verification (HUZZAH32)**

Why manual: smooth optical fade quality (no flicker, no visible duty-cycle steps) requires
human perception. Wokwi confirms register writes but cannot certify perceptual smoothness.

Hardware required: Adafruit HUZZAH32, USB Micro-B cable.

1. Build and flash: `idf.py set-target esp32 && idf.py build flash`
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

1. Build and flash: `idf.py set-target esp32s3 && idf.py build flash`
2. Observe the amber LED on the XIAO (GPIO 21).
3. Confirm LED starts dark, then brightens (first fade is dark → bright per spec).
4. Confirm smooth, continuous breathing over ~4 seconds per cycle.
5. Confirm serial output shows `Fade up` on first fade, then alternating `Fade up` / `Fade down`.

Pass: correct polarity, smooth breathing, correct serial log sequence.

---

**T-M3 — Board Switch Regression**

Why manual: requires physically reflashing two different boards and observing both.

1. Flash HUZZAH32 (esp32 target) — confirm T-M1 passes.
2. Switch to XIAO ESP32S3 (esp32s3 target): `idf.py set-target esp32s3 && idf.py build flash`
3. Confirm T-M2 passes on the XIAO.
4. Switch back to HUZZAH32 — confirm T-M1 still passes.

Pass: both boards pass their visual checks after each switch.

## Spec Files

- [specs/FunctionalSpec.md](specs/FunctionalSpec.md)
- [specs/CodingSpec.md](specs/CodingSpec.md)
- [specs/TestSpec.md](specs/TestSpec.md)

## Board Specs

- [board-specs/adafruit/huzzah32.md](../../board-specs/adafruit/huzzah32.md)
- [board-specs/seeed/xiao-esp32s3.md](../../board-specs/seeed/xiao-esp32s3.md)
- [board-specs/seeed/xiao-esp32c5.md](../../board-specs/seeed/xiao-esp32c5.md)
- [board-specs/seeed/xiao-esp32c6.md](../../board-specs/seeed/xiao-esp32c6.md)
