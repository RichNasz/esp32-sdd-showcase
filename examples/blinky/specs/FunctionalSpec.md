# Breathing LED (Multi-Board)

## Overview
Baseline SDD example. Demonstrates the full spec-to-firmware pipeline using the
ESP-IDF LEDC driver to drive a smooth breathing effect on the onboard status LED.

## Supported Boards
- Adafruit HUZZAH32 (ESP32 Feather)
- Seeed XIAO ESP32S3
- Seeed XIAO ESP32-C5
- Seeed XIAO ESP32-C6

## Requirements
- Breathe the onboard status LED of whichever board is selected.
- Breathing period: 4 seconds (2 s fade-up, 2 s fade-down), continuous.
- Use hardware PWM (ESP-IDF `ledc` driver) — no busy-wait loops.
- Board selection is driven by per-target `sdkconfig.defaults.<target>` files that
  automatically inject the correct LED GPIO, polarity, and console config when the
  user runs `idf.py set-target <chip>`. No menuconfig step is required.
- `idf.py set-target <chip>` alone is sufficient — no menuconfig step required.

## Hardware Dependencies
- Board-spec: board-specs/adafruit/huzzah32.md (ESP32, LED GPIO 13)
- Board-spec: board-specs/seeed/xiao-esp32s3.md (ESP32-S3, LED GPIO 21, active LOW)
- Board-spec: board-specs/seeed/xiao-esp32c5.md (ESP32-C5, LED GPIO 27, active LOW)
- Board-spec: board-specs/seeed/xiao-esp32c6.md (ESP32-C6, LED GPIO 15, active LOW)
- No external components required.

## Success Criteria
- LED breathes smoothly on HUZZAH32 with `idf.py set-target esp32`.
- LED breathes smoothly on XIAO ESP32S3 with `idf.py set-target esp32s3`.
- LED breathes smoothly on XIAO ESP32-C5 with `idf.py set-target esp32c5`.
- LED breathes smoothly on XIAO ESP32-C6 with `idf.py set-target esp32c6`.
- Serial output confirms LEDC timer and fade start.
- Zero compiler warnings.
