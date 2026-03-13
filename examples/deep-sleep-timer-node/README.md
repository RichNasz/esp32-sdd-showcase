<!-- ================================================
     AGENT-GENERATED — DO NOT EDIT BY HAND
     Generated from specs/ using esp32-sdd-documentation-generator skill
     Date: 2026-03-13 | Agent: Claude Code
     ================================================ -->

# Deep Sleep Timer Node — RTC Wakeup + Persistence

> **Power foundation.** Demonstrates the core wake-measure-persist-sleep loop that every battery-powered ESP32 project depends on.

## Overview

An RTC timer wakeup fires every 15 seconds. On each wake, the firmware increments a boot
counter stored in RTC memory (survives deep sleep without NVS overhead), logs the count and
wakeup cause over serial, blinks the onboard LED once as a visible heartbeat, then immediately
returns to deep sleep. The entire active window is under 200 ms.

`RTC_DATA_ATTR` variables survive deep sleep but reset on a full power-cycle — this is expected
and correct behaviour.

## Board

| Item | Detail |
|---|---|
| Board | Seeed XIAO ESP32S3 |
| Target chip | ESP32-S3 |
| ESP-IDF | v5.5.x |
| LED | GPIO 21, active LOW (onboard amber LED) |
| Sleep current | ~14 µA |
| Active window | ~200 ms per cycle |

## Build & Flash

```sh
cd examples/deep-sleep-timer-node
idf.py set-target esp32s3
idf.py build flash monitor
```

## Expected Serial Output

```
I (312) sleep_node: === Wake #1 | cause: first boot ===
I (315) sleep_node: Entering deep sleep for 15 s...

--- (15 seconds pass) ---

I (312) sleep_node: === Wake #2 | cause: timer ===
I (315) sleep_node: Entering deep sleep for 15 s...
```

## Key Concepts

- `esp_sleep_enable_timer_wakeup(15 * 1000000ULL)` — configure RTC timer wakeup source
- `esp_sleep_get_wakeup_cause()` → `ESP_SLEEP_WAKEUP_TIMER` / `ESP_SLEEP_WAKEUP_UNDEFINED`
- `RTC_DATA_ATTR uint32_t boot_count` — lightweight persistence across deep sleep cycles
  without NVS overhead (resets on power-cycle)
- `gpio_hold_dis()` — must be called before driving the LED GPIO on each wake; deep sleep
  can hold GPIO pad states from the previous cycle

## Power Budget

| State | Current | Duration |
|---|---|---|
| Deep sleep | ~14 µA | 15 s |
| Active (CPU + USB-CDC) | ~68 mA | ~200 ms |
| Target average | < 1 mA | — |

## Testing

Testing philosophy: automated first, manual only when hardware observation is unavoidable.

### Automated Tests

1. **T-A1 — Zero-Warning Build**

   ```sh
   cd examples/deep-sleep-timer-node
   idf.py set-target esp32s3 && idf.py build
   ```

   Pass: exit code 0, zero compiler warnings.

2. **T-A2 — Binary Size Check**

   ```sh
   idf.py size | grep "Total binary size"
   ```

   Pass: reported size < 1 MB.

3. **T-A3 — Wokwi Simulation (boot counter + serial log)**

   Justification: simulates RTC timer wakeup cycles and serial output without physical
   hardware, enabling CI verification of the sleep/wake/log loop.

   Prerequisites: [Install Wokwi CLI](https://docs.wokwi.com/wokwi-ci/getting-started);
   `wokwi.toml` + `diagram.json` must be present at the example root.

   ```sh
   wokwi-cli --timeout 60000 --expect "Wake #3" .
   ```

   Pass: `Wake #3` appears in simulated serial output within 60 seconds (3 full 15 s cycles).

### Manual Tests — hardware required

**T-M1 — Boot Counter Persistence Through Real Deep Sleep**

Why manual: `RTC_DATA_ATTR` persistence depends on the hardware power domain staying live
through a true deep-sleep state. Wokwi models this but real silicon is the authoritative test.

1. Flash to XIAO ESP32S3: `idf.py set-target esp32s3 && idf.py build flash monitor`
2. Observe 5 consecutive wakeups — confirm `Wake #1` through `Wake #5` with no gaps.
3. Power-cycle the board (disconnect USB, reconnect).
4. Confirm counter resets: output must show first boot then `Wake #1`.

Pass: counter increments monotonically; resets only on power-cycle.

---

**T-M2 — LED Heartbeat Visibility**

Why manual: visual LED observation (single brief blink per cycle) requires real hardware.

1. Observe the amber LED (GPIO 21) — it should blink exactly once on each wake.
2. LED must remain off during the 15-second sleep period.
3. Blink must be visibly brief (~100 ms), not a long flash.

Pass: single brief blink per cycle, off during sleep.

## Spec Files

- [specs/FunctionalSpec.md](specs/FunctionalSpec.md)
- [specs/CodingSpec.md](specs/CodingSpec.md)
- [specs/TestSpec.md](specs/TestSpec.md)
