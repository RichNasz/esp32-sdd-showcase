# Deep Sleep Timer Node — RTC Wakeup + Persistence

## Overview

Baseline deep-sleep example. Demonstrates the core wake-measure-persist-sleep loop
that every battery-powered ESP32 project depends on. An RTC timer wakeup fires every
15 seconds; the firmware increments a boot counter stored in RTC memory, logs the
count and wakeup cause over serial, blinks the onboard LED once as visible confirmation,
then returns to deep sleep immediately.

On boards with a WS2812 RGB LED (see Supported Boards), each heartbeat blink uses a
randomly selected colour from the project's standard palette — creating an immediately
visible per-cycle variation that is unmistakable in a demo environment.

## Supported Boards

| Board | Manufacturer | SoC | idf.py Target | LED GPIO | LED Type | Polarity | Role | Board-Spec |
|---|---|---|---|---|---|---|---|---|
| Adafruit HUZZAH32 | Adafruit | ESP32 | esp32 | GPIO 13 | Simple GPIO | Active HIGH | sole | `board-specs/adafruit/huzzah32.md` |
| YEJMKJ ESP32-S3-DevKitC-1-N16R8 | YEJMKJ | ESP32-S3 | esp32s3 | GPIO 48 | WS2812 RGB | — | **primary** | `board-specs/yejmkj/esp32-s3-devkitc-1-n16r8.md` |
| Seeed XIAO ESP32S3 | Seeed | ESP32-S3 | esp32s3 | GPIO 21 | Simple GPIO | Active LOW | secondary | `board-specs/seeed/xiao-esp32s3.md` |
| Seeed XIAO ESP32-C5 | Seeed | ESP32-C5 | esp32c5 | GPIO 27 | Simple GPIO | Active LOW | sole | `board-specs/seeed/xiao-esp32c5.md` |
| Espressif ESP32-C6-DevKitC-1-N8 | Espressif | ESP32-C6 | esp32c6 | GPIO 8 | WS2812 RGB | — | **primary** | `board-specs/espressif/esp32-c6-devkitc-1-n8.md` |
| Seeed XIAO ESP32-C6 | Seeed | ESP32-C6 | esp32c6 | GPIO 15 | Simple GPIO | Active LOW | secondary | `board-specs/seeed/xiao-esp32c6.md` |

## Multi-Manufacturer Same-Chip Handling

Two SoC targets are shared by two boards each:

- **esp32s3**: YEJMKJ ESP32-S3-DevKitC-1-N16R8 (WS2812, **primary**) and Seeed XIAO ESP32S3 (GPIO LED, secondary)
- **esp32c6**: Espressif ESP32-C6-DevKitC-1-N8 (WS2812, **primary**) and Seeed XIAO ESP32-C6 (GPIO LED, secondary)

`sdkconfig.defaults.<target>` is configured for the **primary** board on each target. Secondary-board
users must run `idf.py menuconfig` after `idf.py set-target <target>` and set:

- `Example Configuration → Use WS2812 RGB LED (RMT-driven)` → **Disabled**
- `Example Configuration → LED GPIO number` → secondary board GPIO (21 for XIAO ESP32S3; 15 for XIAO ESP32-C6)
- `Example Configuration → LED active level` → **0** (active LOW)
- Console config if needed: XIAO ESP32S3 requires `ESP_CONSOLE_USB_CDC=y`; XIAO ESP32-C6 uses USB Serial/JTAG (default)

## Requirements

- Wake from deep sleep on a 15-second RTC timer wakeup.
- On every wake: increment a `RTC_DATA_ATTR uint32_t boot_count` (survives deep sleep without NVS overhead).
- Detect first boot (`ESP_SLEEP_WAKEUP_UNDEFINED`) vs timer wakeup (`ESP_SLEEP_WAKEUP_TIMER`) and log accordingly.
- Log boot count and wakeup cause over serial on every wake.
- Blink the onboard LED once per wake as a visible heartbeat (100 ms ON, 100 ms OFF).
  - **Simple GPIO boards**: single brief on/off blink.
  - **WS2812 boards**: single brief blink in a randomly selected colour from the project's standard
    6-colour palette (red, green, blue, cyan, magenta, amber — each at 64/255 brightness maximum).
    Colour drawn using hardware RNG (`esp_random()`) each wake cycle; no seeding required.
- Return to deep sleep after ~200 ms active window.
- Zero external components required.

## Hardware Dependencies

All board-specs referenced in the Supported Boards table above. The `esp32-sdd-full-project-generator`
skill auto-invokes:
- `esp32-ws2812-led-engineer` — for WS2812 primary boards (YEJMKJ DevKitC, ESP32-C6-DevKitC)
- `esp32-deep-sleep-engineer` — for RTC timer wakeup configuration across all targets

## Power Budget

| State | Current | Duration |
|---|---|---|
| Deep sleep | ~14 µA | 15 s |
| Active (CPU + USB-CDC) | ~68 mA | ~200 ms |
| Target average | < 1 mA | — |

Note: WS2812 boards (YEJMKJ DevKitC, ESP32-C6-DevKitC) use USB Serial/JTAG or UART bridge
and have no USB-CDC enumeration overhead, so the active window budget is more readily met.

## Success Criteria

- Builds clean for all four targets (esp32, esp32s3, esp32c5, esp32c6) with zero warnings.
- Boot counter increments by 1 on every wakeup cycle with no skips on each board.
- Serial output shows correct wakeup cause and count on every wake.
- GPIO LED boards: LED blinks exactly once per wake cycle; off during sleep.
- WS2812 LED boards: LED blinks exactly once per wake cycle; colour is visibly random across
  consecutive wakeups; LED off during sleep.
- No stack overflows or watchdog triggers across 20+ consecutive cycles on any board.
