# Testing Specification — Deep Sleep Timer Node

## Testing Philosophy

Automated testing using native ESP-IDF tools is strongly preferred.
Additional tools are only allowed with explicit justification and documented benefits.
Manual testing is used only when no practical automated alternative exists.

---

## Automated Tests

### T-A1a — Zero-Warning Build: esp32 (Adafruit HUZZAH32)

**Tool**: `idf.py` (native)

```sh
cd examples/deep-sleep-timer-node
idf.py set-target esp32 && idf.py build
```

Pass: exit code 0, zero compiler warnings.

---

### T-A1b — Zero-Warning Build: esp32s3 (YEJMKJ DevKitC primary)

**Tool**: `idf.py` (native)

```sh
cd examples/deep-sleep-timer-node
idf.py set-target esp32s3 && idf.py build
```

Pass: exit code 0, zero compiler warnings. Verifies WS2812 RMT path compiles correctly.

---

### T-A1c — Zero-Warning Build: esp32c5 (Seeed XIAO ESP32-C5)

**Tool**: `idf.py` (native)

```sh
cd examples/deep-sleep-timer-node
idf.py set-target esp32c5 && idf.py build
```

Pass: exit code 0, zero compiler warnings.

---

### T-A1d — Zero-Warning Build: esp32c6 (Espressif ESP32-C6-DevKitC primary)

**Tool**: `idf.py` (native)

```sh
cd examples/deep-sleep-timer-node
idf.py set-target esp32c6 && idf.py build
```

Pass: exit code 0, zero compiler warnings. Verifies WS2812 RMT path compiles on C6.

---

### T-A2 — Binary Size Check

**Tool**: `idf.py` (native)

```sh
idf.py size | grep "Total binary size"
```

Pass: reported size < 1 MB.

---

### T-A3 — Wokwi Simulation (boot counter + serial log)

**Tool**: Wokwi CLI — justification: simulates RTC timer wakeup cycles and serial output without physical hardware, enabling CI verification of the sleep/wake/log loop.

Prerequisites: `curl -L https://wokwi.com/ci/install.sh | sh`; `wokwi.toml` + `diagram.json` at example root.

```sh
wokwi-cli --timeout 20000 --expect "Wake #3" .
```

Pass: the string `Wake #3` appears in simulated serial output within 20 seconds (3 full 5-second cycles).

---

## Manual Tests

*Manual tests are used only when automated simulation cannot replicate the exact hardware behaviour.*

### T-M1 — Boot Counter Persistence Through Real Deep Sleep

**Why manual**: `RTC_DATA_ATTR` persistence depends on the actual hardware power domain staying live through a true deep-sleep state. Wokwi models this but real silicon power behaviour is the authoritative test.

**Board selection**: Use the Adafruit HUZZAH32 or the Espressif ESP32-C6-DevKitC-1-N8 (left
USB-C UART bridge port) for this test. Both use a hardware UART bridge chip that stays powered
during deep sleep, so `idf.py monitor` remains connected and shows a gap in output during sleep.

USB-native boards (YEJMKJ DevKitC, XIAO ESP32S3, XIAO ESP32-C5, XIAO ESP32-C6) disconnect from
the host when the ESP32 enters deep sleep — the native USB peripheral is powered off. `idf.py
monitor` will exit on the first sleep entry and cannot observe subsequent wakeups.

1. Flash to Adafruit HUZZAH32: `idf.py set-target esp32 && idf.py build flash monitor`
2. Observe 5 consecutive wakeups — confirm `Wake #1`, `Wake #2`, ..., `Wake #5` with no gaps.
3. Power-cycle the board (disconnect USB, reconnect).
4. Confirm counter resets: output must show `First boot` then `Wake #1`.

Pass: counter increments monotonically; resets only on power-cycle; monitor stays connected across sleep cycles.

---

### T-M2 — LED Heartbeat Visibility (GPIO boards)

**Why manual**: Visual LED observation (on/off timing, single blink per cycle) requires real hardware.

1. Flash to a simple GPIO board (e.g. Adafruit HUZZAH32 or Seeed XIAO ESP32-C5).
2. Observe LED blinks exactly once on each wake.
3. LED must remain off during the 5-second sleep period.
4. Blink duration must be visibly brief (~100 ms) — not a long flash.

Pass: single brief blink per cycle, off during sleep.

---

### T-M3 — WS2812 Random-Colour Heartbeat

**Why manual**: Verifying that the LED colour changes across wakeup cycles requires observing the physical LED. The hardware RNG cannot be meaningfully exercised in simulation.

**Monitor note**: USB-native boards (YEJMKJ DevKitC, all XIAO variants) disconnect from the
host during deep sleep. For this test, observe the LED directly — serial monitoring is not
required. Use the Espressif ESP32-C6-DevKitC-1-N8 (left USB-C UART bridge port) if serial
output during sleep cycles is needed.

1. Flash to a WS2812 board (YEJMKJ ESP32-S3-DevKitC-1-N16R8 or Espressif ESP32-C6-DevKitC-1-N8).
2. Observe 5+ consecutive wakeups by watching the LED.
3. Confirm the LED colour changes at least once across the sequence — the colour must not be
   identical on every cycle. (A single fixed colour across all cycles indicates the RNG is not
   being applied; uniform colour is astronomically unlikely with a 6-colour palette.)
4. Confirm each blink colour is clearly visible (not dim or off) — brightness must be at the
   64/255 level, not 8/255.
5. Confirm the LED returns to off (dark) between blinks and during the 5-second sleep period.

Pass: colour varies across cycles; each blink is clearly visible; LED is off between blinks
and during sleep.
