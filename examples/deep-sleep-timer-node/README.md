<!-- ================================================
     AGENT-GENERATED — DO NOT EDIT BY HAND
     Generated from specs/ using esp32-sdd-full-project-generator skill
     Date: 2026-04-12 | Agent: Claude Code
     ================================================ -->

# Deep Sleep Timer Node

> RTC timer wakeup with boot counter persistence and per-cycle LED heartbeat.

## Overview

This example demonstrates the fundamental wake-measure-persist-sleep loop that every
battery-powered ESP32 project depends on. An RTC timer fires every 15 seconds; the
firmware increments a boot counter stored in RTC memory (no NVS overhead), logs the
count and wakeup cause over serial, blinks the onboard LED as a visible heartbeat, then
immediately returns to deep sleep. On boards with a WS2812 RGB LED (YEJMKJ DevKitC,
ESP32-C6-DevKitC-1-N8), each heartbeat blink uses a randomly selected colour from the
project's standard 6-colour palette via the hardware RNG — a demo-friendly per-cycle
variation that is immediately visible.

## Prerequisites

- **ESP-IDF**: v5.x (tested on v5.5.3)
- **Supported boards** (six boards across four targets):

  | Board | idf.py Target | LED Type | Role |
  |---|---|---|---|
  | Adafruit HUZZAH32 | `esp32` | Simple GPIO (red, GPIO 13) | sole |
  | YEJMKJ ESP32-S3-DevKitC-1-N16R8 | `esp32s3` | WS2812 RGB (GPIO 48) | primary |
  | Seeed XIAO ESP32S3 | `esp32s3` | Simple GPIO (orange, GPIO 21) | secondary |
  | Seeed XIAO ESP32-C5 | `esp32c5` | Simple GPIO (yellow, GPIO 27) | sole |
  | Espressif ESP32-C6-DevKitC-1-N8 | `esp32c6` | WS2812 RGB (GPIO 8) | primary |
  | Seeed XIAO ESP32-C6 | `esp32c6` | Simple GPIO (blue, GPIO 15) | secondary |

- Secondary boards (XIAO ESP32S3, XIAO ESP32-C6): see [Secondary Board Configuration](#secondary-board-configuration).

## Build & Flash

### YEJMKJ ESP32-S3-DevKitC-1-N16R8 (esp32s3 primary — WS2812)

```sh
cd examples/deep-sleep-timer-node
idf.py set-target esp32s3
idf.py build
idf.py -p /dev/cu.usbmodem2101 flash
```

Open the serial monitor in a separate terminal (monitor requires an interactive TTY):

```sh
idf.py monitor
```

> **Note**: The YEJMKJ DevKitC has a single USB-C port exposing two interfaces — flash
> port at `/dev/cu.usbmodem2101` (JTAG/DFU) and monitor output on a second `usbmodem`
> port (USB Serial/JTAG CDC). If the board does not auto-reset, hold **BOOT**, press
> **RST**, release **BOOT**, then flash.
>
> **WS2812 solder jumper**: The RGB LED may have a solder jumper that must be bridged
> before the LED responds to firmware. If no colour appears after flashing, inspect the
> RGB solder pads on the board.

### Adafruit HUZZAH32 (esp32)

```sh
idf.py set-target esp32
idf.py build flash monitor
```

### Seeed XIAO ESP32-C5 (esp32c5)

```sh
idf.py set-target esp32c5
idf.py build flash monitor
```

### Espressif ESP32-C6-DevKitC-1-N8 (esp32c6 primary — WS2812)

```sh
idf.py set-target esp32c6
idf.py build flash monitor
```

Use the **left USB-C port** (UART bridge) for flash and monitor.

### Secondary Board Configuration

For XIAO ESP32S3 or XIAO ESP32-C6, which share a target with a WS2812 primary board:

```sh
idf.py set-target esp32s3   # or esp32c6
idf.py menuconfig
```

In `Example Configuration`, set:
- `Use WS2812 RGB LED` → **Disabled**
- `LED GPIO number` → **21** (XIAO ESP32S3) or **15** (XIAO ESP32-C6)
- `LED active level` → **0** (active LOW)

For XIAO ESP32S3 only, also change the console:
- `Component config → ESP System Settings → Channel for console output` → **USB CDC**

Then build and flash normally.

## Opening in VS Code / Cursor

This folder is a complete, standalone ESP-IDF project. Open **only this folder**
(not the repository root) in VS Code or Cursor so the official ESP-IDF Extension
can detect and configure the project automatically.

**File → Open Folder… → select `examples/deep-sleep-timer-node/`**

This gives you one-click Build / Flash / Monitor / Debug, Menuconfig, and size
analysis from the ESP-IDF commands palette. Do not open the top-level repository
folder as your active workspace while developing this example.

## Expected Serial Output

### First boot (WS2812 board — YEJMKJ DevKitC)

```
I (312) sleep-node: First boot — counter resets to 0 on power-cycle
I (314) sleep-node: WS2812 RMT init on GPIO 48 (10 MHz, GRB order)
I (518) sleep-node: Sleeping for 15 s
```

### Subsequent wakeups

```
I (312) sleep-node: Wake #2 — 15 s timer wakeup
I (313) sleep-node: WS2812 RMT init on GPIO 48 (10 MHz, GRB order)
I (517) sleep-node: Sleeping for 15 s

I (312) sleep-node: Wake #3 — 15 s timer wakeup
I (313) sleep-node: WS2812 RMT init on GPIO 48 (10 MHz, GRB order)
I (517) sleep-node: Sleeping for 15 s
```

Each wake cycle the RGB LED blinks a different colour (red, green, blue, cyan, magenta,
or amber — chosen at random). On simple GPIO boards the LED blinks once in a fixed colour
with no colour information in the log.

### After power-cycle

```
I (312) sleep-node: First boot — counter resets to 0 on power-cycle
```

`RTC_DATA_ATTR` memory is retained across deep sleep but cleared on power-on.

## Key Concepts

- **`RTC_DATA_ATTR`** — places a variable in the RTC slow memory domain, which remains
  powered during deep sleep. The counter survives sleep cycles but resets on power-on.
- **`esp_sleep_get_wakeup_cause()`** — distinguishes first boot (`ESP_SLEEP_WAKEUP_UNDEFINED`)
  from a timer wakeup (`ESP_SLEEP_WAKEUP_TIMER`). Unknown causes are logged as warnings
  and the device proceeds to sleep without aborting.
- **`esp_sleep_enable_timer_wakeup()` + `esp_deep_sleep_start()`** — canonical RTC-timer
  deep sleep sequence; duration expressed in microseconds (15 s = 15 000 000 µs).
- **`gpio_hold_dis()`** — must be called before driving the LED GPIO on each wake. Deep
  sleep latches GPIO pad states; driving a held pin has no effect.
- **WS2812 RMT driver** — the `#if CONFIG_EXAMPLE_LED_WS2812` path initialises an RMT TX
  channel at 10 MHz resolution and uses the bytes encoder for WS2812B single-wire NRZ
  signalling. GRB byte order is mandatory.
- **`esp_random()`** — reads directly from the hardware entropy source (TRNG). No seeding
  required. Used to select a random palette index on every wake cycle.
- **`#if CONFIG_ESP_CONSOLE_USB_CDC`** — USB-CDC boards (XIAO ESP32S3 in secondary mode)
  need ~50 ms after boot for USB enumeration; other console types need no delay.
- **Kconfig-driven board config** — all board differences (GPIO, LED type, polarity,
  console) are expressed as `CONFIG_*` symbols in `sdkconfig.defaults.<target>` or via
  menuconfig. No board names appear in C source.

## Testing

Testing philosophy: automated first, manual only when hardware observation is unavoidable.

### Automated Tests

1. **T-A1a — Zero-warning build (esp32)**

   ```sh
   cd examples/deep-sleep-timer-node
   idf.py set-target esp32 && idf.py build
   ```

   Pass: exit code 0, zero compiler warnings.

2. **T-A1b — Zero-warning build (esp32s3, WS2812 path)**

   ```sh
   idf.py set-target esp32s3 && idf.py build
   ```

   Pass: exit code 0, zero warnings. Verifies the WS2812 RMT code path compiles.

3. **T-A1c — Zero-warning build (esp32c5)**

   ```sh
   idf.py set-target esp32c5 && idf.py build
   ```

   Pass: exit code 0, zero warnings.

4. **T-A1d — Zero-warning build (esp32c6, WS2812 path)**

   ```sh
   idf.py set-target esp32c6 && idf.py build
   ```

   Pass: exit code 0, zero warnings. Verifies the WS2812 RMT path compiles on C6.

5. **T-A2 — Binary size check**

   ```sh
   idf.py size | grep "Total binary size"
   ```

   Pass: reported size < 1 MB.

6. **T-A3 — Wokwi simulation (boot counter + serial log)**

   Justification: simulates RTC timer wakeup cycles and serial output without physical
   hardware, enabling CI verification of the sleep/wake/log loop.

   Prerequisites: `curl -L https://wokwi.com/ci/install.sh | sh`; `wokwi.toml` +
   `diagram.json` at example root.

   ```sh
   wokwi-cli --timeout 60000 --expect "Wake #3" .
   ```

   Pass: `Wake #3` appears in simulated serial output within 60 seconds (3 full 15 s cycles).

### Manual — hardware required

7. **T-M1 — Boot counter persistence through real deep sleep**

   *Why manual*: `RTC_DATA_ATTR` persistence depends on the actual RTC power domain
   staying live through a true hardware deep-sleep state.

   1. Flash to YEJMKJ ESP32-S3-DevKitC-1-N16R8: `idf.py set-target esp32s3 && idf.py -p /dev/cu.usbmodem2101 flash`
   2. Open serial monitor in a separate terminal: `idf.py monitor`
   3. Observe 5 consecutive wakeups — confirm `Wake #1`, `Wake #2`, ..., `Wake #5` with no gaps.
   4. Power-cycle the board (disconnect USB, reconnect).
   5. Confirm counter resets: next output must show `First boot` then `Wake #1`.

   Pass: counter increments monotonically; resets only on power-cycle.

8. **T-M2 — LED heartbeat visibility (GPIO boards)**

   *Why manual*: Visual LED timing requires real hardware.

   1. Flash to a simple GPIO board (e.g. Adafruit HUZZAH32 or Seeed XIAO ESP32-C5).
   2. Observe the LED blinks exactly once per wake cycle.
   3. LED must be off during the 15 s sleep period.
   4. Blink must be visibly brief (~100 ms) — not a long flash.

   Pass: single brief blink per cycle, off during sleep.

9. **T-M3 — WS2812 random-colour heartbeat**

   *Why manual*: Verifying colour randomness across wakeup cycles requires observing the
   physical LED; the hardware RNG cannot be meaningfully exercised in simulation.

   1. Flash to a WS2812 board (YEJMKJ DevKitC or ESP32-C6-DevKitC-1-N8).
   2. Observe 5+ consecutive wakeups.
   3. Confirm the LED colour changes at least once across the sequence. A fixed colour
      across all cycles indicates the RNG is not being applied.
   4. Confirm each blink is clearly visible — brightness must be at 64/255, not dim.
   5. Confirm the LED is off between blinks and during the 15 s sleep period.

   Pass: colour varies across cycles; each blink is clearly visible; LED is off between
   blinks and during sleep.

## Spec Files

- [specs/FunctionalSpec.md](specs/FunctionalSpec.md) — requirements, board table, power budget
- [specs/CodingSpec.md](specs/CodingSpec.md) — architecture, LED code paths, Kconfig symbols
- [specs/TestSpec.md](specs/TestSpec.md) — automated and manual test procedures

---

*Generated by [esp32-sdd-full-project-generator](../../skills/esp32-sdd-full-project-generator/SKILL.md)*
