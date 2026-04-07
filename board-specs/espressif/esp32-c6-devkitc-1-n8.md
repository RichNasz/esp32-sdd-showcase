<!-- ================================================
     AGENT-GENERATED — DO NOT EDIT BY HAND
     Generated from vendor documentation via esp32-board-spec-generator skill
     Source: https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32c6/esp32c6-devkitc-1/index.html
     Date: 2026-04-07 | Agent: Claude Code
     ================================================ -->

# Board Spec — Espressif ESP32-C6-DevKitC-1-N8

## Module / SoC

| Item | Value |
| --- | --- |
| SoC | ESP32-C6 (dual RISC-V: 160 MHz high-performance core + 20 MHz low-power core) |
| Module | ESP32-C6-WROOM-1 (PCB trace antenna); WROOM-1U variant uses U.FL connector |
| Flash | 8 MB SPI NOR (quad-SPI) — N8 suffix |
| PSRAM | None |
| Antenna | Integrated PCB trace antenna (WROOM-1); external U.FL connector (WROOM-1U) |
| Form factor | DevKit — 38-pin DIP-style, breadboard-compatible (two 19-pin rows) |
| Certification | FCC / CE / SRRC |

## Power Management

| Item | Detail |
| --- | --- |
| 3V3 rail | ~500 mA from onboard LDO |
| USB input | Two USB-C ports (see USB section below) — 5 V each |
| Battery input | None — no onboard LiPo charger |
| Deep sleep current | ~7 µA typical (RTC domain off); ~15 µA with RTC domain active |
| Active / Wi-Fi TX current | ~200–350 mA peak during Wi-Fi 6 transmission (per ESP32-C6 datasheet) |

> **Power note for SDD projects**: The ~7–15 µA deep sleep floor makes the C6 suitable for battery-powered applications when powered from an external supply with an on/off switch — note that no onboard LiPo charger is present. The 20 MHz LP core can run ULP-style tasks in light sleep while the HP core is gated off. Use the `esp32-deep-sleep-engineer` skill when targeting battery operation. The ESP32-C6 LP core uses dedicated LP peripherals (LP_UART, LP_I2C, LP_GPIO 0–7) distinct from the ESP32 ULP coprocessor.

## Onboard Peripherals

| Item | GPIO | Polarity | Notes |
| --- | --- | --- | --- |
| RGB LED (WS2812) | GPIO 8 | — | Addressable RGB; GPIO 8 is also a strapping pin — leave floating at reset for safe default |
| Power LED | — | — | 3.3 V rail indicator; hardware-controlled, not GPIO-accessible |
| Boot button | GPIO 0 | Active LOW | Pulls GPIO 0 LOW at reset → serial download mode. Do not drive LOW at reset for normal boot |
| Reset button | RST / CHIP_PU | — | Hardware reset only |

## GPIO & Pin Rules / Warnings

### Strapping Pins (do not drive during reset/boot)

| GPIO | Strapping function | Safe default |
| --- | --- | --- |
| GPIO 0 | Boot mode (LOW = serial download; HIGH = SPI flash boot) | Leave floating or pull HIGH. BOOT button pulls LOW for firmware download |
| GPIO 4 (MTMS) | JTAG / boot configuration — sampled at reset | Leave floating |
| GPIO 5 (MTDI) | JTAG / boot configuration — sampled at reset | Leave floating |
| GPIO 8 | ROM log output enable; also drives onboard RGB LED | Leave floating (internal pull-up → safe; also keeps LED off at reset) |
| GPIO 9 | SDIO boot select — sampled at reset | Leave floating |
| GPIO 15 | SDIO boot log — sampled at reset | Leave floating |

### ADC Channels

The ESP32-C6 has **ADC1 only** — there is no ADC2. This means:

- ADC measurements are **never blocked by Wi-Fi activity** (unlike ESP32/ESP32-S3 where ADC2 was unavailable during Wi-Fi).
- All ADC-capable pins (GPIO 0–7) use ADC1 channels and are safe to use alongside active Wi-Fi.

| GPIO | ADC channel |
| --- | --- |
| GPIO 0 | ADC1_CH0 (also XTAL_32K_P — avoid for ADC if using 32 kHz crystal) |
| GPIO 1 | ADC1_CH1 (also XTAL_32K_N — avoid for ADC if using 32 kHz crystal) |
| GPIO 2 | ADC1_CH2 |
| GPIO 3 | ADC1_CH3 |
| GPIO 4 | ADC1_CH4 (strapping pin MTMS — usable after boot) |
| GPIO 5 | ADC1_CH5 (strapping pin MTDI — usable after boot) |
| GPIO 6 | ADC1_CH6 (default I2C SDA — mux with ADC carefully) |
| GPIO 7 | ADC1_CH7 (default I2C SCL — mux with ADC carefully) |

### USB Serial / Console

The ESP32-C6-DevKitC-1 has **two independent USB-C ports**:

- **Left port — USB-UART bridge**: Connected to UART0 (GPIO 16 TX, GPIO 17 RX) via a USB-UART bridge chip. Use this port for `idf.py flash monitor`. No special sdkconfig entry required.
- **Right port — Native USB Serial/JTAG**: Connected directly to the ESP32-C6 built-in USB JTAG controller (GPIO 12 = D−, GPIO 13 = D+). Use this port for hardware JTAG debugging.

When flashing and monitoring via the **left (UART bridge) port**, the standard UART console is active by default — no sdkconfig change is needed. If you instead route the console through the **right (native USB) port**, add:

```ini
CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y
```

> **Note**: This is `USB_SERIAL_JTAG`, not `USB_CDC` — the ESP32-C6 uses the same peripheral as the ESP32-C3, distinct from the ESP32-S3's full USB OTG controller.

### Reserved / Internal Pins (do not use in user code)

| GPIO | Reason |
| --- | --- |
| GPIO 24–30 | Internal quad-SPI flash interface; not exposed on package pads |
| GPIO 12–13 | Native USB JTAG D−/D+; avoid in user code if hardware JTAG debugging is required |

## Complete Pin Mapping Table

### User-Accessible Pads (J1 left header + J3 right header)

| Header | GPIO | ADC | SPI (FSPI) | I2C | UART | LP peripheral | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- |
| J1 — GPIO 4 (MTMS) | GPIO 4 | ADC1_CH4 | — | — | — | — | Strapping pin; usable after boot completes |
| J1 — GPIO 5 (MTDI) | GPIO 5 | ADC1_CH5 | — | — | — | — | Strapping pin; usable after boot completes |
| J1 — GPIO 6 (MTCK) | GPIO 6 | ADC1_CH6 | FSPI CLK | I2C SDA | — | — | Default I2C SDA |
| J1 — GPIO 7 (MTDO) | GPIO 7 | ADC1_CH7 | FSPI D0 | I2C SCL | — | — | Default I2C SCL |
| J1 — GPIO 15 | GPIO 15 | — | — | — | — | — | Strapping pin; general I/O after boot |
| J1 — GPIO 16 (TX) | GPIO 16 | — | — | — | UART0 TX | — | Connected to USB-UART bridge (left USB-C) |
| J1 — GPIO 17 (RX) | GPIO 17 | — | — | — | UART0 RX | — | Connected to USB-UART bridge (left USB-C) |
| J1 — GPIO 18 | GPIO 18 | — | FSPI CLK alt | — | — | — | General I/O |
| J1 — GPIO 19 | GPIO 19 | — | FSPI D1 | — | — | — | General I/O |
| J1 — GPIO 20 | GPIO 20 | — | FSPI D2 | — | — | — | General I/O |
| J1 — GPIO 21 | GPIO 21 | — | — | — | — | — | General I/O |
| J3 — GPIO 0 | GPIO 0 | ADC1_CH0 | — | — | — | LP_GPIO0 | Strapping pin; BOOT button; XTAL_32K_P alt |
| J3 — GPIO 1 | GPIO 1 | ADC1_CH1 | — | — | — | LP_GPIO1 | XTAL_32K_N alt; LP core accessible |
| J3 — GPIO 2 | GPIO 2 | ADC1_CH2 | FSPI Q | — | — | LP_GPIO2 | LP core accessible |
| J3 — GPIO 3 | GPIO 3 | ADC1_CH3 | FSPI HD | — | UART1 RX | LP_GPIO3 | LP core accessible |
| J3 — GPIO 9 | GPIO 9 | — | — | — | — | — | Strapping pin; general I/O after boot |
| J3 — GPIO 10 | GPIO 10 | — | FSPI CS | — | — | — | General I/O |
| J3 — GPIO 11 | GPIO 11 | — | FSPI WP | — | — | — | General I/O |
| J3 — GPIO 12 | GPIO 12 | — | — | — | — | — | Native USB JTAG D−; avoid if using JTAG |
| J3 — GPIO 13 | GPIO 13 | — | — | — | — | — | Native USB JTAG D+; avoid if using JTAG |
| J3 — GPIO 22 | GPIO 22 | — | — | — | — | — | General I/O |
| J3 — GPIO 23 | GPIO 23 | — | — | — | — | — | General I/O |

### Internal / Special GPIOs

| GPIO | Function | Driveable by user? |
| --- | --- | --- |
| GPIO 8 | Onboard RGB LED (WS2812) / strapping pin | Yes — primary onboard RGB LED after boot |
| GPIO 0 | Boot button / strapping (download mode) | Only after boot completes |
| GPIO 12 | Native USB JTAG D− | Only if not using hardware JTAG |
| GPIO 13 | Native USB JTAG D+ | Only if not using hardware JTAG |
| GPIO 24–30 | Internal SPI flash | No |

## Board Advantages for SDD Projects

- **ADC1-only architecture**: No ADC2 means all analog reads are safe during active Wi-Fi — no need to gate ADC around network activity (unlike ESP32 or ESP32-S3).
- **Dual RISC-V cores**: The 20 MHz LP core can run ULP-style tasks in light sleep while the 160 MHz HP core is gated off — useful for sensor polling without full wakeup cycles.
- **Wi-Fi 6 (802.11ax)**: TWT (Target Wake Time) enables IoT power budgeting; reference this in FunctionalSpec when designing battery-efficient connected devices.
- **Zigbee + Thread (802.15.4)**: Native Matter-over-Thread support built into the SoC — specify protocol in FunctionalSpec when building smart home mesh devices.
- **Dual USB-C ports**: Independent UART bridge (left) and native JTAG (right) — no swapping cables between flash and debug workflows.
- **Onboard WS2812 RGB LED**: Single-wire addressable LED on GPIO 8; useful for status indication without external hardware.
- **8 MB flash (N8)**: Comfortable headroom for OTA dual-bank partition layouts; sufficient for most connectivity + sensor workloads.
- **No PSRAM**: Budget memory usage carefully in FunctionalSpec — 512 KB HP SRAM is the ceiling; avoid JPEG frame buffers or large ML models without external SPI RAM.
- **Deep sleep ~7–15 µA**: Suitable for battery-powered applications via external power supply; note no onboard LiPo charger — external charging circuit required for LiPo.
- **Breadboard-compatible DIP form factor**: 38 pins across two rows; works directly on standard 830-point breadboards.

## idf.py Target

```sh
idf.py set-target esp32c6
```

> When using the **left USB-C port** (UART bridge), the default UART console is active — no extra sdkconfig entry is needed. When routing the console through the **right USB-C port** (native USB JTAG), add `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y` to `sdkconfig.defaults`.
