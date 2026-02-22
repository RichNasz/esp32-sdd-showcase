<!-- ================================================
     AGENT-GENERATED — DO NOT EDIT BY HAND
     Generated from vendor documentation via esp32-board-spec-generator skill
     Source: https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/
     Date: 2026-02-22 | Agent: Claude Code
     ================================================ -->

# Board Spec — Seeed XIAO ESP32S3

## Module / SoC

| Item | Value |
|---|---|
| SoC | ESP32-S3R8 (dual-core Xtensa LX7, up to 240 MHz) |
| Flash | 8 MB SPI NOR (quad-SPI) |
| PSRAM | 8 MB OPI PSRAM (octal-SPI, high bandwidth) |
| Antenna | Integrated ceramic antenna + U.FL connector (switch via software) |
| Form factor | XIAO 14-pin (21 × 17.8 mm) |
| Certification | FCC / CE |

## Power Management

| Item | Detail |
|---|---|
| 3V3 rail | Up to 700 mA from onboard LDO |
| USB input | USB-C, 5 V |
| Battery input | JST 1.25 mm connector; onboard LiPo charger (100 mA charge rate) |
| Deep sleep current | ~14 µA (typical, with all peripherals off) |
| Active current | ~68 mA (Wi-Fi transmitting) |

> **Power note for SDD projects**: Always include `esp_deep_sleep_start()` and configure wakeup sources in the FunctionalSpec when targeting battery operation. The ~14 µA deep sleep floor makes this board suitable for multi-month battery life with infrequent wakeups.

## Onboard Peripherals

| Item | GPIO | Polarity | Notes |
|---|---|---|---|
| User LED (orange) | GPIO 21 | **Active LOW** | LED is ON when GPIO21 = LOW; OFF when HIGH |
| Charge indicator LED | — | — | Separate physical LED; not GPIO-controlled |
| Boot button | GPIO 0 | Active LOW | Also a strapping pin — do not drive at boot |
| Reset button | EN | — | Hardware reset only |

**LED note**: GPIO21 is the only user-controllable LED on this board. It is always active LOW — driving GPIO21 HIGH turns the LED off; driving it LOW turns it on. LEDC fade must use inverted duty: duty = 0 means full brightness, duty = max means fully off.

## GPIO & Pin Rules / Warnings

### Strapping Pins (do not drive during reset/boot)

| GPIO | Strapping function | Safe default |
|---|---|---|
| GPIO 0 | Boot mode selector (HIGH = normal boot, LOW = download mode) | Leave floating or pull HIGH |
| GPIO 45 | VDD_SPI voltage (LOW = 3.3 V, HIGH = 1.8 V) | Leave floating (defaults to 3.3 V) |
| GPIO 46 | ROM log verbosity (LOW = silent) | Leave floating |

### ADC Restrictions

- **ADC1** (GPIO1–GPIO10): Available at all times, including during Wi-Fi operation.
- **ADC2** (GPIO11–GPIO20): **Disabled when Wi-Fi is active.** All exposed user pads (D0–D10) use ADC1 channels, so they are unaffected by Wi-Fi. Avoid using GPIO11–GPIO20 for ADC when Wi-Fi is enabled.

### USB-CDC Serial

The XIAO ESP32S3 uses native USB (not a USB-UART bridge). Serial monitor requires:
```
CONFIG_ESP_CONSOLE_USB_CDC=y
```
in `sdkconfig.defaults`. Without this, `idf.py monitor` will show no output.

### SPI Flash / PSRAM Pins (reserved — do not use)

GPIO 35–37 are used internally for the SPI flash interface and must not be driven by user code.

## Complete Pin Mapping Table

### User-Accessible Pads (D0–D10)

| D-pin | GPIO | ADC | SPI | I2C | UART | Notes |
|---|---|---|---|---|---|---|
| D0 / A0 | GPIO 1 | ADC1_CH0 | — | — | — | General I/O |
| D1 / A1 | GPIO 2 | ADC1_CH1 | — | — | — | General I/O |
| D2 / A2 | GPIO 3 | ADC1_CH2 | — | — | — | General I/O |
| D3 / A3 | GPIO 4 | ADC1_CH3 | — | — | — | General I/O |
| D4 / SDA | GPIO 5 | ADC1_CH4 | — | I2C SDA | — | Default I2C SDA |
| D5 / SCL | GPIO 6 | ADC1_CH5 | — | I2C SCL | — | Default I2C SCL |
| D6 / TX | GPIO 43 | — | — | — | UART0 TX | USB-CDC TX when CDC enabled |
| D7 / RX | GPIO 44 | — | — | — | UART0 RX | USB-CDC RX when CDC enabled |
| D8 / SCK | GPIO 7 | ADC1_CH6 | SPI SCK | — | — | Default SPI clock |
| D9 / MISO | GPIO 8 | ADC1_CH7 | SPI MISO | — | — | Default SPI MISO |
| D10 / MOSI | GPIO 9 | ADC1_CH8 | SPI MOSI / CS | — | — | Default SPI MOSI |

### Internal / Special GPIOs

| GPIO | Function | Driveable by user? |
|---|---|---|
| GPIO 21 | Onboard orange LED (active LOW) | Yes — primary user LED |
| GPIO 0 | Boot button / strapping | Only after boot |
| GPIO 35–37 | Internal SPI flash | No |
| GPIO 45 | VDD_SPI strapping | No |
| GPIO 46 | ROM log strapping | No |

## Board Advantages for SDD Projects

- **Tiny footprint** (21 × 17.8 mm) suits space-constrained enclosures; breadboard-compatible.
- **8 MB OPI PSRAM** provides high-bandwidth memory for ML inference buffers, JPEG frames, or audio DSP — reference this in FunctionalSpec memory requirements.
- **Dual antenna** (ceramic built-in + U.FL external) allows antenna selection in sdkconfig for range-sensitive applications.
- **Deep sleep ~14 µA** makes multi-month coin-cell or small LiPo operation feasible; combine with `esp32-deep-sleep-engineer` skill.
- **ESP32-S3 vector extensions** (AIE) accelerate neural network operations — note in FunctionalSpec when using TensorFlow Lite for Microcontrollers.
- **ADC1 on all exposed pads** means ADC measurements are Wi-Fi safe on all user pins.
- **LiPo charger built-in** — no external charging circuit needed; reference the JST 1.25 mm connector in power budget specs.

## idf.py Target

```sh
idf.py set-target esp32s3
```

> Remember to also set `CONFIG_ESP_CONSOLE_USB_CDC=y` in `sdkconfig.defaults` for serial output over USB-C.
