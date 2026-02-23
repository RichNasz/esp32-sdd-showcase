# Deep Sleep BME280 + MQTT — Real IoT Sensor Node

## Overview

Production IoT pattern. Wakes from deep sleep, reads temperature, humidity, and pressure
from a BME280 sensor over I2C, connects to Wi-Fi, publishes a JSON payload via MQTT,
then returns to deep sleep — all within a 15-second active window. Demonstrates the
minimal wake → sense → transmit → sleep loop used in battery-powered sensor networks.
A button on GPIO 0 triggers an immediate wakeup for on-demand readings.

## Supported Boards

- Seeed XIAO ESP32S3 (default)

## Requirements

- Wake from deep sleep on a 15-second RTC timer wakeup (maximum active window: 15 s).
- Read temperature (°C), relative humidity (%), and pressure (hPa) from BME280 over I2C.
- Connect to Wi-Fi using credentials stored in `sdkconfig.defaults` (CONFIG_WIFI_SSID / CONFIG_WIFI_PASSWORD).
- Publish a single MQTT message in JSON format:
  `{"temp":22.5,"hum":48.2,"pres":1013.1,"boot":42}`
  to topic `sdd/sensor/bme280` on a configurable broker (CONFIG_MQTT_BROKER_URI).
- Disconnect from Wi-Fi and MQTT cleanly before sleeping to flush TCP state.
- GPIO 0 (boot button, active LOW) configured as ext1 wakeup source for immediate on-demand readings.
- Blink LED (GPIO 21, active LOW) once after successful MQTT publish as confirmation.
- Increment `RTC_DATA_ATTR uint32_t boot_count` on every wake.
- Return to deep sleep after publish (or after timeout if Wi-Fi / broker unreachable).

## Hardware Dependencies

- Board-spec: board-specs/seeed/xiao-esp32s3.md (ESP32-S3, I2C SDA=GPIO5, SCL=GPIO6, LED GPIO 21 active LOW)
- External: BME280 breakout board connected to D4 (SDA, GPIO 5) and D5 (SCL, GPIO 6), 3.3 V power.

## Power Budget

| State | Current | Duration |
|---|---|---|
| Deep sleep | ~14 µA | 15 s |
| Active + Wi-Fi + MQTT | ~68 mA | ≤ 15 s |
| Target average (15 s cycle) | < 5 mA | — |

## Connectivity

- Wi-Fi: station mode (STA), WPA2-Personal.
- MQTT: broker URI configurable via CONFIG_MQTT_BROKER_URI (e.g. `mqtt://192.168.1.100`).
- Protocol: MQTT 3.1.1, QoS 0, no TLS (TLS variant covered in secure-ota-https example).

## Success Criteria

- BME280 readings are non-zero and within plausible ranges (10–40 °C, 10–90 %, 900–1100 hPa).
- MQTT message arrives at broker within 15-second active window on every cycle.
- Device returns to deep sleep successfully after publish.
- Button press (GPIO 0) triggers immediate wakeup and reading.
- No Wi-Fi association failures across 10+ consecutive cycles in stable RF conditions.
