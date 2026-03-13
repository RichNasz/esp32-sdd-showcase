<!-- ================================================
     AGENT-GENERATED — DO NOT EDIT BY HAND
     Generated from vendor documentation using esp32-board-spec-generator skill
     Source: Espressif ESP32-S3-DevKitC-1 v1.1 User Guide
     Date: 2026-03-10 | Agent: Claude Code
     ================================================ -->

# Board Spec — YEJMKJ ESP32-S3-DevKitC-1-N16R8

## Module / SoC

| Item | Value |
| --- | --- |
| SoC | ESP32-S3R8 (dual-core Xtensa LX7, up to 240 MHz, AI vector extensions) |
| Module | ESP32-S3-WROOM-1-N16R8 |
| Flash | 16 MB Quad SPI NOR |
| PSRAM | 8 MB OPI PSRAM |
| Antenna | PCB trace (integrated, 2.4 GHz) |
| Form factor | DevKit — 38-pin dual-header (J1 left, J3 right), breadboard-compatible |
| Certification | CE / FCC (via ESP32-S3-WROOM-1 module) |

## Power Management

| Item | Detail |
| --- | --- |
| 3V3 rail | ~600 mA max from onboard LDO (5 V input via either USB-C) |
| USB-C input | ESP32-S3 native USB OTG — 5 V, power + data; exposes JTAG/DFU + CDC/console |
| Battery input | None |
| Deep sleep current | ~7 µA typical (RTC timer only, all peripherals off) |
| Active current | ~80–240 mA during Wi-Fi transmission |

> **Power note for SDD projects**: Deep sleep is fully viable for battery operation via GPIO0–21 (all RTC-capable on ESP32-S3). Use the `esp32-deep-sleep-engineer` skill to design wakeup sources. Note that the 8 MB OPI PSRAM is not retained during deep sleep and must be re-initialised on wakeup.

## Onboard Peripherals

| Item | GPIO | Polarity | Notes |
| --- | --- | --- | --- |
| RGB LED (WS2812) | GPIO48 | Active HIGH signal | Addressable LED on GPIO48. **Solder jumper required** — if the LED is non-functional out of the box, check and bridge the RGB solder pads on the board. |
| Boot button | GPIO0 | Active LOW | Strapping pin — hold LOW during reset to enter download mode; do not drive at boot |
| Reset button | EN/RST | Active LOW (resets SoC) | Located on same side as USB-to-Serial port and Boot button |

## GPIO & Pin Rules / Warnings

### Strapping Pins (do not drive during reset/boot)

| GPIO | Strapping function | Safe default |
| --- | --- | --- |
| GPIO0 | Boot mode: HIGH = normal boot, LOW = serial download mode | Leave floating or pull HIGH (onboard pull-up present) |
| GPIO3 | JTAG signal source: LOW = use pads for JTAG, HIGH = not JTAG | Leave floating |
| GPIO45 | VDD_SPI voltage: LOW = 3.3 V, HIGH = 1.8 V | Leave floating (defaults to 3.3 V; N16R8 uses 3.3 V) |
| GPIO46 | ROM serial output: LOW = suppress ROM log, HIGH = enable | Leave floating (enables ROM log by default) |

### ADC Restrictions

ADC1 (GPIO1–GPIO10, channels CH0–CH9) is always available and safe to use alongside Wi-Fi and BLE.

ADC2 (GPIO11–GPIO20, channels CH0–CH9) **cannot be used when Wi-Fi is active**. If your application uses Wi-Fi, use only ADC1 channels for analog measurements.

Touch sensor channels (TOUCH1–TOUCH14) overlap with GPIO1–GPIO14 and share the RTC domain.

### USB Serial / Console

The board has **one USB-C port** connected directly to the ESP32-S3 native USB OTG peripheral (GPIO19 USB\_D−, GPIO20 USB\_D+). There is **no CH343P** or any other USB-to-UART bridge chip on this board. Over the single USB-C connector the ESP32-S3 exposes two logical USB interfaces simultaneously:

- **USB JTAG/DFU interface** — used for flashing. Enumerates as `/dev/cu.usbmodem*2101` (macOS) or `/dev/ttyACM0` (Linux). Pass this port to `idf.py -p <port> flash`.
- **USB Serial/JTAG CDC interface** — used for serial console output. Enumerates as a second `/dev/cu.usbmodem*` device (macOS) or `/dev/ttyACM1` (Linux). Pass this port to `idf.py -p <port> monitor`.

Add the following to `sdkconfig.defaults` for all SDD projects targeting this board:

```ini
CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y
```

**Use a data-capable USB-C cable** — charge-only cables have no data lines and will prevent the board from being detected entirely.

### Reserved / Internal Pins (do not use in user code)

| GPIO | Reason |
| --- | --- |
| GPIO26–GPIO32 | Internal Quad SPI flash interface (ESP32-S3-WROOM-1 module) |
| GPIO35–GPIO37 | Internal OPI PSRAM interface (8 MB R8 variant) |

## Complete Pin Mapping Table

### J1 Header — Left Side (top to bottom)

| Label | GPIO | ADC | Touch | Notes |
| --- | --- | --- | --- | --- |
| 3V3 (×2) | — | — | — | 3.3 V power output |
| RST | EN | — | — | Active-LOW chip reset |
| IO4 | GPIO4 | ADC1\_CH3 | TOUCH4 | RTC\_GPIO4 |
| IO5 | GPIO5 | ADC1\_CH4 | TOUCH5 | RTC\_GPIO5 |
| IO6 | GPIO6 | ADC1\_CH5 | TOUCH6 | RTC\_GPIO6 |
| IO7 | GPIO7 | ADC1\_CH6 | TOUCH7 | RTC\_GPIO7 |
| IO15 | GPIO15 | ADC2\_CH4 | — | RTC\_GPIO15; U0RTS; XTAL\_32K\_P |
| IO16 | GPIO16 | ADC2\_CH5 | — | RTC\_GPIO16; U0CTS; XTAL\_32K\_N |
| IO17 | GPIO17 | ADC2\_CH6 | — | RTC\_GPIO17; U1TXD |
| IO18 | GPIO18 | ADC2\_CH7 | — | RTC\_GPIO18; U1RXD; CLK\_OUT3 |
| IO8 | GPIO8 | ADC1\_CH7 | TOUCH8 | RTC\_GPIO8; SUBSPICS1 |
| IO3 | GPIO3 | ADC1\_CH2 | TOUCH3 | RTC\_GPIO3; strapping pin |
| IO46 | GPIO46 | — | — | Strapping pin (ROM log enable) |
| IO9 | GPIO9 | ADC1\_CH8 | TOUCH9 | RTC\_GPIO9; FSPIHD |
| IO10 | GPIO10 | ADC1\_CH9 | TOUCH10 | RTC\_GPIO10; FSPICS0 |
| IO11 | GPIO11 | ADC2\_CH0 | TOUCH11 | RTC\_GPIO11; FSPID |
| IO12 | GPIO12 | ADC2\_CH1 | TOUCH12 | RTC\_GPIO12; FSPICLK |
| IO13 | GPIO13 | ADC2\_CH2 | TOUCH13 | RTC\_GPIO13; FSPIQ |
| IO14 | GPIO14 | ADC2\_CH3 | TOUCH14 | RTC\_GPIO14; FSPIWP |
| 5V | — | — | — | 5 V power output |
| GND | — | — | — | Ground |

### J3 Header — Right Side (top to bottom)

| Label | GPIO | ADC | Notes |
| --- | --- | --- | --- |
| GND | — | — | Ground |
| TX | GPIO43 | — | U0TXD; CLK\_OUT1 |
| RX | GPIO44 | — | U0RXD; CLK\_OUT2 |
| IO1 | GPIO1 | ADC1\_CH0 | RTC\_GPIO1; TOUCH1 |
| IO2 | GPIO2 | ADC1\_CH1 | RTC\_GPIO2; TOUCH2 |
| IO42 | GPIO42 | — | MTMS (JTAG) |
| IO41 | GPIO41 | — | MTDI (JTAG); CLK\_OUT1 |
| IO40 | GPIO40 | — | MTDO (JTAG); CLK\_OUT2 |
| IO39 | GPIO39 | — | MTCK (JTAG); CLK\_OUT3; SUBSPICS1 |
| IO38 | GPIO38 | — | FSPIWP; SUBSPIWP (free-use; no onboard peripheral) |
| IO37 | GPIO37 | — | **Reserved** — OPI PSRAM internal |
| IO36 | GPIO36 | — | **Reserved** — OPI PSRAM internal |
| IO35 | GPIO35 | — | **Reserved** — OPI PSRAM internal |
| IO0 | GPIO0 | — | RTC\_GPIO0; Boot button; strapping pin |
| IO45 | GPIO45 | — | Strapping pin (VDD\_SPI voltage) |
| IO48 | GPIO48 | — | **RGB LED (WS2812)** — SPICLK\_N; SUBSPICLK\_N\_DIFF. Solder jumper may need bridging for LED to function. |
| IO47 | GPIO47 | — | SPICLK\_P; SUBSPICLK\_P\_DIFF |
| IO21 | GPIO21 | — | RTC\_GPIO21 |
| IO20 | GPIO20 | ADC2\_CH9 | RTC\_GPIO20; U1CTS; USB\_D+; native USB OTG positive |
| IO19 | GPIO19 | ADC2\_CH8 | RTC\_GPIO19; U1RTS; USB\_D−; native USB OTG negative |
| GND (×2) | — | — | Ground |

### Internal / Special GPIOs

| GPIO | Function | Driveable by user? |
| --- | --- | --- |
| GPIO26–GPIO32 | Internal Quad SPI flash (WROOM-1 module) | No |
| GPIO35–GPIO37 | Internal OPI PSRAM (8 MB R8) | No |
| GPIO48 | Onboard RGB LED (WS2812, active HIGH signal) — solder jumper must be bridged if non-functional | Yes |
| GPIO0 | Boot button (active LOW) | Yes — with caution (strapping pin) |
| GPIO19 | Native USB D− | Yes — only when USB OTG is not in use |
| GPIO20 | Native USB D+ | Yes — only when USB OTG is not in use |

## Board Advantages for SDD Projects

- Deep sleep floor of ~7 µA makes long battery life achievable; all GPIO0–21 are RTC-capable on ESP32-S3, giving flexible wakeup options.
- 8 MB OPI PSRAM enables large ML inference buffers, JPEG frame capture, audio DSP pipelines, and LittleFS/FATFS overlays without flash wear.
- 16 MB flash provides ample space for OTA partitions, large file systems, and complex multi-component applications.
- Single USB-C port uses the ESP32-S3 native USB OTG peripheral directly — no CH343P bridge. It simultaneously exposes a JTAG/DFU interface for flashing and a USB Serial/JTAG CDC interface for console, with no extra drivers required on modern macOS/Linux.
- Onboard WS2812 RGB LED (GPIO48) is useful for status indication in any example without additional hardware. Note: solder jumper on the board may need bridging before the LED functions.
- 4 dedicated JTAG pins (GPIO39–42) allow hardware-assisted debugging via `idf.py openocd` without occupying user-accessible GPIOs.
- ADC1 (10 channels, GPIO1–10) is always available, even when Wi-Fi/BLE is active — safe for sensor applications that coexist with wireless.
- ESP32-S3 AI vector extensions (PIE) accelerate TFLite Micro inference; combined with 8 MB PSRAM, the board is suitable for on-device ML demos.

## Flashing Procedure

Connect via the single **USB-C port**. The board presents two `usbmodem` interfaces — identify both before flashing:

- Flash port: `/dev/cu.usbmodem*2101` (USB JTAG/DFU)
- Monitor port: `/dev/cu.usbmodem*5A7B*` (USB Serial/JTAG CDC)

Run flash and monitor together:

```sh
idf.py -p /dev/cu.usbmodem2101 flash -p /dev/cu.usbmodem5A7B1597911 monitor
```

If the board does not enumerate, enter bootloader mode manually: hold **BOOT** (GPIO0), press and release **RST**, then release **BOOT**. Native USB JTAG does not use DTR/RTS auto-reset — the manual sequence may be needed on first flash.

## idf.py Target

```sh
idf.py set-target esp32s3
```

> This board has **one USB-C port** (native USB OTG, no CH343P). It enumerates as two `usbmodem` devices: a JTAG/DFU interface for flashing and a USB Serial/JTAG CDC interface for console. Always set `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y` in `sdkconfig.defaults`. Pass each port explicitly: `idf.py -p <jtag-port> flash -p <cdc-port> monitor`. Do NOT set `CONFIG_ESP_CONSOLE_USB_CDC=y`.
