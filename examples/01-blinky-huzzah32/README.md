<!-- ================================================
     AGENT-GENERATED — DO NOT EDIT BY HAND
     Generated from specs/ using esp32-sdd-documentation-generator skill
     Date: 2026-02-22 | Agent: Claude Code
     ================================================ -->

# 01 · Blinky HUZZAH32 — PWM Breathing LED

> **Baseline example.** Master the SDD project structure and the ESP-IDF LEDC driver before advancing to more complex examples.

## What It Demonstrates

- SDD project layout: `FunctionalSpec.md` → `CodingSpec.md` → fully generated project
- ESP-IDF `ledc` driver for hardware PWM output (no `vTaskDelay` busy-loops)
- Smooth breathing effect via LEDC fade interrupt callbacks
- GPIO safety conventions for the Adafruit HUZZAH32 (ESP32 Feather)

## Target Hardware

| Item | Detail |
|---|---|
| Board | Adafruit HUZZAH32 (ESP32 Feather) |
| LED | Built-in red LED — GPIO 13 |
| Power | USB or 3.7 V LiPo via JST connector |

## Build & Flash

```sh
cd examples/01-blinky-huzzah32
idf.py set-target esp32
idf.py build flash monitor
```

## Expected Output

```
I (320) blinky: LEDC timer configured — 5 kHz, 13-bit resolution
I (322) blinky: Fade started — breathing period: 4 s
```

The LED breathes smoothly through a full 4-second cycle, indefinitely.

## Key Concepts

- `ledc_timer_config_t` / `ledc_channel_config_t`
- `ledc_fade_func_install()` + `ledc_set_fade_with_time()`
- `ledc_cb_register()` — interrupt-driven fade completion callback
- Agent-generated `CMakeLists.txt` and `sdkconfig.defaults` structure

## Spec Files

- [FunctionalSpec.md](FunctionalSpec.md)
- [CodingSpec.md](CodingSpec.md)
