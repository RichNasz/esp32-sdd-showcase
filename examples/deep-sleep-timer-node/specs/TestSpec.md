# Testing Specification — Deep Sleep Timer Node

## Testing Philosophy

Automated testing using native ESP-IDF tools is strongly preferred.
Additional tools are only allowed with explicit justification and documented benefits.
Manual testing is used only when no practical automated alternative exists.

---

## Automated Tests

### T-A1 — Zero-Warning Build

**Tool**: `idf.py` (native)

```sh
cd examples/deep-sleep-timer-node
idf.py set-target esp32s3 && idf.py build
```

Pass: exit code 0, zero compiler warnings.

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
wokwi-cli --timeout 60000 --expect "Wake #3" .
```

Pass: the string `Wake #3` appears in simulated serial output within 60 seconds (3 full 15-second cycles).

---

## Manual Tests

*Manual tests are used only when automated simulation cannot replicate the exact hardware behaviour.*

### T-M1 — Boot Counter Persistence Through Real Deep Sleep

**Why manual**: `RTC_DATA_ATTR` persistence depends on the actual hardware power domain staying live through a true deep-sleep state. Wokwi models this but real silicon power behaviour is the authoritative test.

1. Flash to XIAO ESP32S3: `idf.py set-target esp32s3 && idf.py build flash`
2. Open serial monitor: `idf.py monitor`
3. Observe 5 consecutive wakeups — confirm `Wake #1`, `Wake #2`, ..., `Wake #5` with no gaps.
4. Power-cycle the board (disconnect USB, reconnect).
5. Confirm counter resets: next output must show `First boot` then `Wake #1`.

Pass: counter increments monotonically; resets only on power-cycle.

---

### T-M2 — LED Heartbeat Visibility

**Why manual**: Visual LED observation (on/off timing, single blink per cycle) requires real hardware.

1. Observe amber LED (GPIO 21) blinks exactly once on each wake.
2. LED must remain off during the 15-second sleep period.
3. Blink duration must be visibly brief (~100 ms) — not a long flash.

Pass: single brief blink per cycle, off during sleep.
