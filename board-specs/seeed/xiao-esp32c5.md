<!-- ================================================
     AGENT-GENERATED — DO NOT EDIT BY HAND
     Generated from vendor documentation via esp32-board-spec-generator skill
     Source: https://www.seeedstudio.com/Seeed-Studio-XIAO-ESP32C5-p-6609.html
             https://wiki.seeedstudio.com/xiao_esp32c5_getting_started/
     Date: 2026-03-08 | Agent: Claude Code
     ================================================ -->

# Board Spec — Seeed XIAO ESP32-C5

## Module / SoC

| Item | Value |
| --- | --- |
| SoC | ESP32-C5 (single-core RISC-V 32-bit, up to 240 MHz) |
| Module | Bare SoC (no separate module; SoC mounted directly on XIAO PCB) |
| Flash | 8 MB SPI NOR (quad-SPI) |
| PSRAM | 8 MB |
| Antenna | External antenna via U.FL connector (antenna included in box) |
| Form factor | XIAO (21 × 17.8 mm), single-sided mount, breadboard-compatible |
| Certification | FCC / CE (verify current status on Seeed product page) |

## Power Management

| Item | Detail |
| --- | --- |
| 3V3 rail | ~500 mA from onboard LDO (verify exact rating in Seeed schematic) |
| USB input | USB-C, 5 V |
| Battery input | JST 1.25 mm 2-pin connector; onboard LiPo charger (SGM40567 IC; charge indicated by red LED) |
| Deep sleep current | ~8 µA typical (verify against ESP32-C5 datasheet §Power Consumption) |
| Active / Wi-Fi TX current | ~80–100 mA average during Wi-Fi 6 transmission (verify against ESP32-C5 datasheet) |

> **Power note for SDD projects**: The low deep sleep floor makes this board suitable for battery-powered applications with infrequent wakeups. The ESP32-C5's dual-band Wi-Fi 6 supports Target Wake Time (TWT) for power-efficient connected operation. Use the `esp32-deep-sleep-engineer` skill when targeting battery operation. GPIO and timer wakeup sources are confirmed supported; avoid using JTAG pads (MTMS/MTDI/MTCK/MTDO) as wakeup sources to preserve debugging capability.

## Onboard Peripherals

| Item | GPIO | Polarity | Notes |
| --- | --- | --- | --- |
| User LED (yellow) | GPIO 27 | **Active LOW** | LED ON when GPIO27 = LOW; OFF when HIGH |
| Charge indicator LED | — | — | Red LED; hardware-controlled by SGM40567 charger IC; not GPIO-accessible |
| Boot button | GPIO 9 | Active LOW | Pulls GPIO9 LOW at reset → serial download mode. Do not drive LOW at reset for normal boot |
| Reset button | EN / CHIP_PU | — | Hardware reset only |
| Battery voltage sense | GPIO 6 | — | ADC input; reads battery voltage via resistor divider |
| Battery sense enable | GPIO 26 | Active HIGH | Drive HIGH to enable the battery voltage measurement circuit; drive LOW when idle to save power |

## GPIO & Pin Rules / Warnings

### Strapping Pins (do not drive during reset/boot)

| GPIO | Strapping function | Safe default |
| --- | --- | --- |
| GPIO 8 | USB JTAG / boot mode (HIGH = USB JTAG enabled; LOW = disabled) | Leave floating — internal pull-up holds HIGH |
| GPIO 9 | Boot mode selector (LOW = serial download mode; HIGH = SPI flash boot) | Leave floating or pull HIGH. BOOT button pulls LOW for firmware download |

> **Note**: D8 (GPIO8) and D9 (GPIO9) are user-accessible pads that double as strapping pins. Do not drive them LOW at reset from external circuitry. After boot completes, both are freely usable as GPIO.

### ADC

The ESP32-C5 exposes **5 ADC-capable GPIOs** on the XIAO pads, all on ADC1. ADC2 channels, if present on the SoC, are not routed to user pads.

- ADC measurements on the exposed pads are **not blocked by Wi-Fi activity** — all user ADC pins use ADC1.
- GPIO6 is used internally as the battery voltage monitor input; avoid repurposing it for other ADC tasks when battery sensing is active.

| GPIO | XIAO label | ADC |
| --- | --- | --- |
| GPIO 1 | D0 / A0 | ADC1 (verify channel number from ESP32-C5 datasheet) |
| GPIO 2 | MTMS | ADC1 (verify channel number) |
| GPIO 3 | MTDI | ADC1 (verify channel number) |
| GPIO 4 | MTCK | ADC1 (verify channel number) |
| GPIO 6 | ADC_BAT | ADC1 (battery sense; verify channel number) |

### USB Serial / Console

The ESP32-C5 uses a **built-in USB Serial/JTAG controller** — no external USB-UART bridge chip is present. Serial monitor output requires:

```ini
CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y
```

in `sdkconfig.defaults`. Without this, `idf.py monitor` will show no output over USB-C.

> **Note**: This is `USB_SERIAL_JTAG`, not `USB_CDC` — the ESP32-C5 uses the same peripheral family as the ESP32-C3 and ESP32-C6, distinct from the ESP32-S3's full USB OTG (`USB_CDC`).

### JTAG Pads (reverse side of PCB)

| XIAO label | GPIO | Notes |
| --- | --- | --- |
| MTDO | GPIO 5 | JTAG TDO; also LP_UART_TXD, LP_GPIO5 |
| MTDI | GPIO 3 | JTAG TDI; also LP_I2C_SCL, LP_GPIO3; ADC-capable |
| MTCK | GPIO 4 | JTAG TCK; also LP_UART_RXD, LP_GPIO4; ADC-capable |
| MTMS | GPIO 2 | JTAG TMS; also LP_I2C_SDA, LP_GPIO2; ADC-capable |

> **Warning**: Do not use MTMS/MTDI/MTCK/MTDO (GPIO2–5) as deep sleep wakeup sources. Doing so disables hardware debugging and can make reflashing difficult. (Per Seeed Studio XIAO ESP32-C5 wiki.)

### Reserved / Internal Pins (do not use in user code)

| GPIO | Reason |
| --- | --- |
| GPIO 13–22 (approximate) | Internal SPI flash and PSRAM interface — not exposed on XIAO pads; exact range must be verified from ESP32-C5 datasheet pin table |
| GPIO 26 | Battery sense enable — reserved for power management; treat as system-owned |

## Complete Pin Mapping Table

### User-Accessible Pads (D0–D10 and JTAG header)

| Label | GPIO | ADC | SPI | I2C | UART | LP peripheral | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- |
| D0 / A0 | GPIO 1 | ADC1 | — | — | — | LP_UART_DSRN, LP_GPIO1 | General I/O |
| D1 | GPIO 0 | — | — | — | — | LP_UART_DTRN, LP_GPIO0 | General I/O |
| D2 | GPIO 25 | — | — | — | — | — | General I/O |
| D3 | GPIO 7 | — | — | — | — | SDIO_DATA1 | General I/O; SDIO alt function |
| D4 / SDA | GPIO 23 | — | — | I2C SDA | — | — | Default I2C SDA |
| D5 / SCL | GPIO 24 | — | — | I2C SCL | — | — | Default I2C SCL |
| D6 / TX | GPIO 11 | — | — | — | UART TX | — | Serial TX |
| D7 / RX | GPIO 12 | — | — | — | UART RX | — | Serial RX |
| D8 / SCK | GPIO 8 | — | SPI CLK | — | — | TOUCH7 | Default SPI clock; **strapping pin** — see warning above |
| D9 / MISO | GPIO 9 | — | SPI MISO | — | — | TOUCH8 | Default SPI MISO; **strapping pin** / BOOT button |
| D10 / MOSI | GPIO 10 | — | SPI MOSI | — | — | TOUCH9 | Default SPI MOSI |
| MTDO | GPIO 5 | — | — | — | — | LP_UART_TXD, LP_GPIO5 | JTAG pad (reverse side) |
| MTDI | GPIO 3 | ADC1 | — | — | — | LP_I2C_SCL, LP_GPIO3 | JTAG pad; ADC-capable |
| MTCK | GPIO 4 | ADC1 | — | — | — | LP_UART_RXD, LP_GPIO4 | JTAG pad; ADC-capable |
| MTMS | GPIO 2 | ADC1 | — | — | — | LP_I2C_SDA, LP_GPIO2 | JTAG pad; ADC-capable |

### Internal / Special GPIOs

| GPIO | Function | Driveable by user? |
| --- | --- | --- |
| GPIO 6 | Battery voltage ADC sense (ADC_BAT) | Yes — but treat as system-owned when battery sensing is enabled |
| GPIO 9 | Boot button / strapping pin | Only after boot completes |
| GPIO 26 | Battery sense circuit enable (ADC_CTL) | Yes — drive HIGH to enable measurement, LOW when idle |
| GPIO 27 | Onboard user LED (active LOW, yellow) | Yes — primary user LED |
| GPIO 13–22 (approx.) | Internal SPI flash / PSRAM | No |

## Board Advantages for SDD Projects

- **Dual-band Wi-Fi 6 (2.4 GHz + 5 GHz)**: First XIAO with 5 GHz support — reduces interference in crowded 2.4 GHz environments; specify band preference in FunctionalSpec when range or throughput is a priority.
- **Wi-Fi 6 TWT (Target Wake Time)**: Enables power-efficient connected operation; reference in FunctionalSpec for battery-powered IoT with periodic cloud sync.
- **Bluetooth 5 LE + Mesh**: Full BLE 5.0 with mesh networking; useful for multi-device sensor networks.
- **8 MB Flash + 8 MB PSRAM**: Generous storage and RAM for data logging, OTA buffers, audio DSP, or small ML models — reference in FunctionalSpec memory requirements.
- **ADC1-only on exposed pads**: No ADC2 restrictions during Wi-Fi; all analog reads are safe alongside active networking.
- **LP peripheral access on JTAG pads**: GPIO2–5 expose LP_I2C and LP_UART — useful for ultra-low-power sensor polling in light sleep without the main CPU.
- **LiPo charger built-in** (SGM40567): No external charging circuit needed; status visible from charge indicator LED; battery voltage readable via GPIO6.
- **Tiny XIAO footprint** (21 × 17.8 mm): Drop-in replacement for other XIAO boards; share enclosure, PCB, and mounting designs across the XIAO family.
- **Security hardware**: AES-128/256, SHA, HMAC, and Secure Boot V2 available — reference `esp32-hardware-crypto-configurer` skill when security is required.

> **ESP-IDF compatibility note**: ESP32-C5 support was added in ESP-IDF v5.3. It is not listed in the project's `shared-specs/esp-idf-version.md` supported targets table — update that file before using this board in a project example. Pin in `sdkconfig.defaults` as `CONFIG_IDF_TARGET="esp32c5"`.

## idf.py Target

```sh
idf.py set-target esp32c5
```

> Remember to set `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y` in `sdkconfig.defaults` for serial output over USB-C. Without this line, `idf.py monitor` produces no output.
