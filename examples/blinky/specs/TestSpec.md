# Testing Specification — Blinky (PWM Breathing LED + WS2812)

## Testing Philosophy

Automated testing using native ESP-IDF tools is strongly preferred.
Additional tools are only allowed with explicit justification and documented benefits.
Manual testing is used only when no practical automated alternative exists.

---

## Automated Tests

### T-A1 — Zero-Warning Build (HUZZAH32)

**Tool**: `idf.py` (native, no extras required)
**What it verifies**: The generated project compiles cleanly for the Adafruit HUZZAH32
(esp32 target, LEDC path) with no warnings or errors.

```sh
cd examples/blinky
rm -f sdkconfig sdkconfig.old
idf.py set-target esp32
idf.py build 2>&1 | grep -E "warning:|error:|ninja: build stopped"
# Expected: no output (zero warnings, zero errors)
```

Pass criterion: exit code 0 from `idf.py build`, grep produces no output.

---

### T-A2 — Zero-Warning Build (XIAO ESP32S3)

**Tool**: `idf.py` (native, no extras required)
**What it verifies**: The generated project compiles cleanly for the Seeed XIAO ESP32S3
(esp32s3 target, LEDC path, active-LOW polarity) with no warnings or errors.

```sh
cd examples/blinky
rm -f sdkconfig sdkconfig.old
idf.py set-target esp32s3
idf.py build 2>&1 | grep -E "warning:|error:|ninja: build stopped"
# Expected: no output (zero warnings, zero errors)
```

Pass criterion: exit code 0 from `idf.py build`, grep produces no output.

---

### T-A3 — Zero-Warning Build (ESP32-C6-DevKitC — WS2812 path)

**Tool**: `idf.py` (native, no extras required)
**What it verifies**: The WS2812/RMT code path compiles cleanly for the Espressif
ESP32-C6-DevKitC-1-N8 (esp32c6 target) with no warnings or errors.

```sh
cd examples/blinky
rm -f sdkconfig sdkconfig.old
idf.py set-target esp32c6
idf.py build 2>&1 | grep -E "warning:|error:|ninja: build stopped"
# Expected: no output (zero warnings, zero errors)
```

Pass criterion: exit code 0 from `idf.py build`, grep produces no output.

---

### T-A4 — sdkconfig Verification (DevKitC)

**Tool**: `grep` on the generated sdkconfig (native)
**What it verifies**: After `set-target esp32c6` the correct Kconfig symbols are applied —
WS2812 code path enabled, correct GPIO, UART bridge console (no USB_SERIAL_JTAG). This guards
against stale sdkconfig files shadowing the updated `sdkconfig.defaults.esp32c6`.

```sh
# Run immediately after T-A3 (reuses its sdkconfig)
grep -E "EXAMPLE_LED_WS2812|EXAMPLE_LED_GPIO|ESP_CONSOLE_USB_SERIAL_JTAG" sdkconfig
```

Expected output:
```
CONFIG_EXAMPLE_LED_GPIO=8
CONFIG_EXAMPLE_LED_WS2812=y
# CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG is not set
```

Pass criterion: `EXAMPLE_LED_WS2812=y`, `EXAMPLE_LED_GPIO=8`, and
`CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG` is not set (or absent).

---

### T-A5 — Binary Size Sanity Check

**Tool**: `idf.py` (native)
**What it verifies**: The firmware binary fits well within flash constraints (< 1 MB).

```sh
idf.py size | grep "Total binary size"
# Expected: < 1,048,576 bytes
```

Pass criterion: reported binary size < 1 MB.

---

## Manual Tests

*Manual tests are used only when no practical automated alternative exists. Each step explains
why automation is not applicable.*

### T-M1 — Visual LED Breathing Verification (HUZZAH32)

**Why manual**: Smooth optical fade quality (no flicker, no duty-cycle steps visible to the
eye) requires human perception. A build check confirms register writes but cannot certify
perceptual smoothness.

Hardware required: Adafruit HUZZAH32, USB Micro-B cable.

Steps:
1. `rm -f sdkconfig sdkconfig.old && idf.py set-target esp32 && idf.py build flash`
2. Observe the red LED on the HUZZAH32 (GPIO 13).
3. Confirm: LED fades from off → fully on → off smoothly over approximately 4 seconds per cycle.
4. Confirm: no visible steps or flicker during the fade.
5. Confirm: cycle repeats indefinitely without stopping.

Pass criterion: observer confirms smooth, continuous breathing with no visible artefacts.

---

### T-M2 — Visual LED Breathing Verification (XIAO ESP32S3)

**Why manual**: Same as T-M1. Additionally, the XIAO uses active-LOW polarity — manual
verification confirms the DUTY_ON/DUTY_OFF inversion is correct on real silicon.

Hardware required: Seeed XIAO ESP32S3, USB-C cable.

Steps:
1. `rm -f sdkconfig sdkconfig.old && idf.py set-target esp32s3 && idf.py build flash`
2. Observe the amber LED on the XIAO (GPIO 21).
3. Confirm: LED is dark at startup, then brightens (first fade direction is dark → bright per spec).
4. Confirm: smooth, continuous breathing over approximately 4 seconds per cycle.
5. Confirm: serial output shows `Fade up` on first fade, then alternating `Fade up` / `Fade down`.

Pass criterion: observer confirms correct polarity, smooth breathing, and correct serial log sequence.

---

### T-M3 — Visual WS2812 Breathing Verification (ESP32-C6-DevKitC-1-N8)

**Why manual**: WS2812 breathing is a software loop driving the RMT peripheral. Visual
confirmation verifies the protocol is correctly timed and the LED responds — any protocol
error or wrong code path produces no light or a solid-on condition that build checks cannot detect.

Hardware required: Espressif ESP32-C6-DevKitC-1-N8, USB-C cable connected to the **left port**
(UART bridge, appears as `/dev/cu.usbserial-*` on macOS).

Steps:
1. `rm -f sdkconfig sdkconfig.old && idf.py set-target esp32c6 && idf.py build`
2. `idf.py flash -p /dev/cu.usbserial-*`
3. In a terminal: `idf.py monitor -p /dev/cu.usbserial-*`
4. Confirm serial output: `WS2812 RMT channel init on GPIO 8` then `Starting WS2812 breathing loop`.
5. Observe the onboard RGB LED.
6. Confirm: LED breathes white (dim → bright → dim) with no stuck-on or stuck-off condition.
7. Confirm: breathing period is approximately 4 seconds per cycle.

Pass criterion: LED breathes white smoothly; serial log confirms RMT init and loop start.

---

### T-M4 — Secondary Board Override (Seeed XIAO ESP32-C6)

**Why manual**: Verifies the menuconfig override path works for secondary boards that share
a chip target with the primary board. Cannot be automated without a two-board CI rig that
also validates visual LED behaviour.

Hardware required: Seeed XIAO ESP32-C6, USB-C cable.

Steps:
1. `rm -f sdkconfig sdkconfig.old && idf.py set-target esp32c6`
2. `idf.py menuconfig` → navigate to **Blinky Configuration** → set:
   - `LED is a WS2812B...` → **disabled** (`EXAMPLE_LED_WS2812=n`)
   - `LED GPIO number` → **15** (`EXAMPLE_LED_GPIO=15`)
   - Save and exit.
3. `idf.py build && idf.py flash`
4. Confirm serial output shows LEDC init (`freq=5000 Hz`), **not** WS2812 init.
5. Observe the onboard LED on GPIO 15 breathing smoothly.

Pass criterion: LEDC breathing on GPIO 15; no `WS2812 RMT` in serial log.

---

### T-M5 — Board Switch Regression

**Why manual**: Requires physically reflashing two different boards and observing both.
Cannot be automated without a two-board CI rig.

Steps:
1. Flash HUZZAH32 (esp32 target) — confirm T-M1 passes.
2. `rm -f sdkconfig sdkconfig.old && idf.py set-target esp32s3 && idf.py build flash`
3. Confirm T-M2 passes on the XIAO ESP32S3.
4. Switch back to HUZZAH32 and repeat — confirm T-M1 still passes.

Pass criterion: both boards pass their respective visual checks after each switch.
