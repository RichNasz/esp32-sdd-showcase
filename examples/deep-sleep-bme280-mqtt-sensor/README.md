<!-- ================================================
     AGENT-GENERATED — DO NOT EDIT BY HAND
     Generated from specs/ using esp32-sdd-documentation-generator skill
     Date: 2026-03-13 | Agent: Claude Code
     ================================================ -->

# Deep Sleep BME280 + MQTT — Real IoT Sensor Node

> **Production IoT pattern.** Minimal wake → sense → transmit → sleep loop used in battery-powered sensor networks.

## Overview

Wakes from deep sleep every 15 seconds, reads temperature, humidity, and pressure from a BME280
sensor over I2C, connects to Wi-Fi, publishes a JSON payload via MQTT to a configurable broker,
then returns to deep sleep — all within a 15-second active window. A button on GPIO 0 triggers
an immediate on-demand reading. Failure at any pipeline stage (sensor, Wi-Fi, MQTT) causes the
device to skip remaining stages and enter deep sleep immediately — it never blocks indefinitely.

## Board

| Item | Detail |
|---|---|
| Board | Seeed XIAO ESP32S3 (default) |
| Target chip | ESP32-S3 |
| ESP-IDF | v5.5.x |
| I2C SDA | GPIO 5 (D4) |
| I2C SCL | GPIO 6 (D5) |
| LED | GPIO 21, active LOW |
| Button (ext1 wakeup) | GPIO 0 (boot button, active LOW) |

## Hardware Wiring

| Component | XIAO Pin | GPIO | Notes |
|---|---|---|---|
| BME280 SDA | D4 | GPIO 5 | 3.3 V I2C; 400 kHz |
| BME280 SCL | D5 | GPIO 6 | |
| BME280 VCC | 3V3 | — | |
| BME280 GND | GND | — | |

Use a BME280 breakout board (Adafruit, SparkFun, or similar). No resistors required —
the XIAO and most breakouts have built-in I2C pull-ups.

## Build & Flash

```sh
cd examples/deep-sleep-bme280-mqtt-sensor
idf.py set-target esp32s3
idf.py menuconfig   # set Wi-Fi SSID/Password and MQTT broker URI
idf.py build flash monitor
```

## Expected Serial Output

```
I (312) sensor_node: Wake #3 | cause: timer
I (315) sensor_node: BME280: temp=22.5°C  hum=48.2%  pres=1013.1 hPa
I (318) sensor_node: Wi-Fi connected — IP: 192.168.1.55
I (412) sensor_node: MQTT published: {"temp":22.5,"hum":48.2,"pres":1013.1,"boot":3}
I (415) sensor_node: LED blink — publish confirmed
I (615) sensor_node: Entering deep sleep for 15 s...
```

## Key Concepts

- `esp_sleep_enable_timer_wakeup` + `esp_sleep_enable_ext1_wakeup` — dual wakeup sources
  (timer every 15 s; GPIO 0 button for on-demand readings)
- Bosch reference BME280 driver via `idf_component.yml` (ESP-IDF Component Registry) — handles
  calibration compensation correctly without a bare I2C driver
- ESP-IDF 5.x `i2c_master` new driver API (not the legacy `i2c_driver_install` pattern)
- BME280 forced mode — single measurement, then standby, minimising I2C active time
- `esp_mqtt_client` — built-in MQTT 3.1.1, QoS 0, no TLS (sufficient for LAN sensors)
- Linear pipeline with per-stage timeouts; failure skips remaining stages and sleeps
- `RTC_DATA_ATTR uint32_t boot_count` — persistent cycle counter across deep sleep

## Power Budget

| State | Current | Duration |
|---|---|---|
| Deep sleep | ~14 µA | 15 s |
| Active + Wi-Fi + MQTT | ~68 mA | ≤ 15 s |
| Target average (15 s cycle) | < 5 mA | — |

## Testing

Testing philosophy: automated first, manual only when hardware observation is unavoidable.

### Automated Tests

1. **T-A1 — Zero-Warning Build**

   ```sh
   cd examples/deep-sleep-bme280-mqtt-sensor
   idf.py set-target esp32s3 && idf.py build
   ```

   Pass: exit code 0, zero compiler warnings.

2. **T-A2 — Binary Size Check**

   ```sh
   idf.py size | grep "Total binary size"
   ```

   Pass: < 2 MB (Wi-Fi + MQTT stack included).

3. **T-A3 — Component Dependency Resolution**

   ```sh
   idf.py reconfigure 2>&1 | grep -i "error"
   ```

   Pass: no errors; `bosch/bme280` component resolves from the ESP-IDF component registry.

### Manual Tests — hardware required

**T-M1 — BME280 Sensor Reads Valid Data**

Why manual: real I2C communication and sensor calibration coefficients require physical hardware.

1. Wire BME280 breakout: SDA→D4, SCL→D5, VCC→3V3, GND→GND.
2. Flash and open serial monitor.
3. Confirm log shows temperature 10–40 °C, humidity 10–90 %, pressure 900–1100 hPa.
4. Breathe on the sensor — confirm humidity reading increases.

Pass: all three readings in valid range; humidity responds to breath test.

---

**T-M2 — MQTT Publish Reaches Broker**

Why manual: end-to-end MQTT delivery requires a live broker and Wi-Fi AP.

1. Start a local MQTT broker (e.g. `mosquitto`).
2. Subscribe: `mosquitto_sub -t "sdd/sensor/bme280"`.
3. Flash and monitor — confirm message arrives at broker with valid JSON.
4. Confirm `boot` counter increments on each wake.

Pass: broker receives well-formed JSON on every cycle.

---

**T-M3 — Button Wakeup**

Why manual: physical GPIO 0 button press triggers ext1 wakeup — requires real hardware.

1. During deep sleep, press and release the BOOT button (GPIO 0).
2. Confirm immediate wakeup and serial log shows `ext1` wakeup cause.
3. Confirm a full sense + publish cycle completes before returning to sleep.

Pass: button triggers wakeup and full cycle completes.

---

**T-M4 — Active Window Timeout**

Why manual: requires deliberately misconfiguring Wi-Fi credentials to force a timeout.

1. Set `CONFIG_WIFI_SSID` to an invalid SSID.
2. Flash and monitor.
3. Confirm device logs a timeout warning and enters deep sleep within 15 seconds — no hang.

Pass: device sleeps within 15 seconds even when network is unavailable.

## Spec Files

- [specs/FunctionalSpec.md](specs/FunctionalSpec.md)
- [specs/CodingSpec.md](specs/CodingSpec.md)
- [specs/TestSpec.md](specs/TestSpec.md)
