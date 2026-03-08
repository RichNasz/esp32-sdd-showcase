<!-- ================================================
     AGENT-GENERATED — DO NOT EDIT BY HAND
     Generated from specs/ using esp32-sdd-documentation-generator skill
     Date: 2026-03-08 | Agent: Claude Code
     ================================================ -->

# Board Spec — <!-- FILL: Vendor Board/Module Name -->

## Module / SoC

| Item | Value |
| --- | --- |
| SoC | <!-- FILL: e.g. ESP32-S3R8 (dual-core Xtensa LX7, up to 240 MHz) --> |
| Module | <!-- FILL: e.g. ESP32-WROOM-32 , or "Bare SoC (no module)" --> |
| Flash | <!-- FILL: e.g. 8 MB SPI NOR (quad-SPI) --> |
| PSRAM | <!-- FILL: e.g. 8 MB OPI PSRAM, or "None" --> |
| Antenna | <!-- FILL: e.g. PCB trace / ceramic / U.FL connector --> |
| Form factor | <!-- FILL: e.g. Feather, XIAO, DevKit, or module dimensions --> |
| Certification | <!-- FILL: e.g. FCC / CE, or "—" if unknown --> |

## Power Management

| Item | Detail |
| --- | --- |
| 3V3 rail | <!-- FILL: e.g. 500 mA max from AP2112 LDO --> |
| USB input | <!-- FILL: e.g. USB-C, 5 V --> |
| Battery input | <!-- FILL: e.g. JST 1.25 mm LiPo connector with onboard charger, or "None" --> |
| Deep sleep current | <!-- FILL: e.g. ~14 µA typical with all peripherals off --> |
| Active current | <!-- FILL: e.g. ~68 mA during Wi-Fi transmission --> |

> **Power note for SDD projects**: <!-- FILL: Note whether deep sleep is viable for battery operation, reference the esp32-deep-sleep-engineer skill if so. -->

## Onboard Peripherals

| Item | GPIO | Polarity | Notes |
| --- | --- | --- | --- |
| <!-- FILL: e.g. User LED --> | <!-- FILL: GPIO N --> | <!-- FILL: Active HIGH / Active LOW --> | <!-- FILL: any sharing or usage notes --> |
| <!-- FILL: e.g. Boot button --> | <!-- FILL: GPIO N --> | <!-- FILL: Active LOW --> | <!-- FILL: also a strapping pin — do not drive at boot --> |

## GPIO & Pin Rules / Warnings

### Strapping Pins (do not drive during reset/boot)

| GPIO | Strapping function | Safe default |
| --- | --- | --- |
| <!-- FILL: GPIO N --> | <!-- FILL: e.g. Boot mode selector --> | <!-- FILL: e.g. Leave floating or pull HIGH --> |

### ADC Restrictions

<!-- FILL: List which ADC banks are available and any restrictions during Wi-Fi operation.
Example: ADC2 is disabled when Wi-Fi is active. ADC1 channels are always available. -->

### USB Serial / Console

<!-- FILL: Note whether the board uses a USB-UART bridge (e.g. CP2104) or native USB-CDC.
If native USB: include the required sdkconfig.defaults line in an ```ini code block:
  CONFIG_ESP_CONSOLE_USB_CDC=y          (ESP32-S2/S3 USB OTG)
  CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y  (ESP32-C3/C6 USB Serial/JTAG)
If bridge: include the bridge chip name and TX/RX GPIO assignments.
NOTE: Always specify a language on fenced code blocks (e.g. ```ini, ```sh, ```c). -->

### Reserved / Internal Pins (do not use in user code)

| GPIO | Reason |
| --- | --- |
| <!-- FILL: GPIO N–M --> | <!-- FILL: e.g. Internal SPI flash interface --> |

## Complete Pin Mapping Table

### User-Accessible Pads

| Label | GPIO | ADC | SPI | I2C | UART | Notes |
| --- | --- | --- | --- | --- | --- | --- |
| <!-- FILL: D0 / A0 --> | GPIO <!-- N --> | <!-- ADC1_CHn or — --> | <!-- — --> | <!-- — --> | <!-- — --> | <!-- FILL --> |

### Internal / Special GPIOs

| GPIO | Function | Driveable by user? |
| --- | --- | --- |
| <!-- FILL: GPIO N --> | <!-- FILL: e.g. Onboard LED (active LOW) --> | <!-- FILL: Yes / No --> |

## Board Advantages for SDD Projects

<!-- FILL: Bullet list of hardware features that are relevant when authoring a FunctionalSpec.md.
Typical items to consider:
- Deep sleep floor and battery life implications
- PSRAM size and use cases (ML buffers, JPEG frames, audio DSP)
- Antenna options and range trade-offs
- Built-in LiPo charger
- SoC vector extensions (S3 AIE for TFLite)
- ADC1-safe pads during Wi-Fi
- Form factor advantages (size, breadboard compatibility)
-->

- <!-- FILL -->

## idf.py Target

```sh
idf.py set-target <!-- FILL: esp32 | esp32s2 | esp32s3 | esp32c3 | esp32c6 | esp32h2 -->
```

<!-- FILL: Add any required sdkconfig.defaults reminders here, e.g.:
> Remember to set `CONFIG_ESP_CONSOLE_USB_CDC=y` in `sdkconfig.defaults` for USB-C serial output.
-->
