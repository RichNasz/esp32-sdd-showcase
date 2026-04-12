# Breathing LED (Multi-Board)

## Overview
Baseline SDD example. Demonstrates the full spec-to-firmware pipeline. Drives a smooth
breathing effect on the onboard status LED using either the ESP-IDF LEDC driver (hardware
PWM, for simple GPIO LEDs) or the RMT peripheral (800 kHz NRZ protocol, for WS2812 RGB LEDs),
selected at compile time via a Kconfig bool. Covers four boards across three manufacturers.

## Supported Boards
- Adafruit HUZZAH32 (ESP32 Feather)
- Seeed XIAO ESP32S3
- Seeed XIAO ESP32-C5
- Espressif ESP32-C6-DevKitC-1-N8 (default C6 target; WS2812 RGB LED)

Note: Seeed XIAO ESP32-C6 is also compatible but requires manual menuconfig override
(set EXAMPLE_LED_WS2812=n, EXAMPLE_LED_GPIO=15) since both share the esp32c6 IDF target.

## Requirements
- Breathe the onboard status LED of whichever board is selected.
- Breathing period: 4 seconds (2 s fade-up, 2 s fade-down), continuous.
- For simple GPIO LED boards: use hardware PWM (ESP-IDF `ledc` driver) — no busy-wait loops.
- For WS2812 RGB LED boards (DevKitC): use RMT peripheral to drive 800 kHz NRZ protocol;
  breathing is implemented as RGB brightness cycling in a software loop; the color advances
  through a fixed sequence (red, green, blue, cyan, magenta, amber) at the start of each new
  breath cycle.
- Board selection is driven by per-target `sdkconfig.defaults.<target>` files that
  automatically inject the correct LED GPIO, LED type, and console config when the
  user runs `idf.py set-target <chip>`. No menuconfig step is required for listed boards.

## Hardware Dependencies
- Board-spec: board-specs/adafruit/huzzah32.md (ESP32, LED GPIO 13, active HIGH, simple GPIO)
- Board-spec: board-specs/seeed/xiao-esp32s3.md (ESP32-S3, LED GPIO 21, active LOW, simple GPIO)
- Board-spec: board-specs/seeed/xiao-esp32c5.md (ESP32-C5, LED GPIO 27, active LOW, simple GPIO)
- Board-spec: board-specs/espressif/esp32-c6-devkitc-1-n8.md (ESP32-C6, LED GPIO 8, WS2812 RGB)
- DevKitC: uses left USB-C port (UART bridge on UART0) — no special console sdkconfig required.
- No external components required.

## Multi-Manufacturer Same-Chip Handling

When two boards from different manufacturers share the same ESP32 chip variant they also share
the same IDF target name (e.g. `esp32c6`) and therefore the same `sdkconfig.defaults.esp32c6`
file. Only one such file can exist per target. This example resolves the conflict as follows:

**Primary board per target** (what `sdkconfig.defaults.<target>` is configured for):
- `esp32c6` → Espressif ESP32-C6-DevKitC-1-N8 (GPIO 8, WS2812 RGB)

**Secondary boards** (same chip, different hardware — require one additional step):
- Seeed XIAO ESP32-C6 (GPIO 15, simple active-LOW LED):
  after `idf.py set-target esp32c6`, run `idf.py menuconfig` →
  Blinky Configuration → set `EXAMPLE_LED_WS2812=n`, `EXAMPLE_LED_GPIO=15`,
  then build and flash as normal.

**General rule for adding a new board of an existing target chip:**
1. Identify what hardware differs (LED type, GPIO, console peripheral).
2. Express each difference as a Kconfig symbol (bool for type, int for GPIO/polarity).
3. Update `sdkconfig.defaults.<target>` to reflect the primary board.
4. Document secondary boards and their menuconfig overrides here.
5. Never create board-specific #ifdefs in C source — all board differences live in Kconfig.

**Why this matters:** `idf.py set-target` re-reads `sdkconfig.defaults.*` from scratch.
An old `sdkconfig` on disk will shadow the defaults. Always `rm -f sdkconfig sdkconfig.old`
before switching boards or after updating `sdkconfig.defaults.*` files.

## Success Criteria
- LED breathes smoothly on HUZZAH32 with `idf.py set-target esp32`.
- LED breathes smoothly on XIAO ESP32S3 with `idf.py set-target esp32s3`.
- LED breathes smoothly on XIAO ESP32-C5 with `idf.py set-target esp32c5`.
- WS2812 RGB LED breathes white smoothly on ESP32-C6-DevKitC-1-N8 with `idf.py set-target esp32c6`.
- Serial output confirms LED driver init (LEDC or RMT) and breathing loop start.
- Zero compiler warnings.
