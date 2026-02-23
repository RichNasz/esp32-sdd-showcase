<!-- ================================================
     AGENT-GENERATED — DO NOT EDIT BY HAND
     Generated from specs/ using esp32-sdd-documentation-generator skill
     Date: 2026-02-22 | Agent: Claude Code
     ================================================ -->

# Deep Sleep BME280 + MQTT — Real IoT Sensor Node

> **Production IoT pattern.** Combines sensor reading, Wi-Fi acquisition, MQTT publish, and deep sleep into a disciplined power-managed measurement loop.

## What It Demonstrates

- BME280 temperature / humidity / pressure reading over I2C
- Minimal Wi-Fi connect → MQTT publish → disconnect sequence
- Deep sleep between measurement cycles for low average power
- Retained MQTT messages for Home Assistant / Node-RED integration
- Fast Wi-Fi reconnect using stored credentials in NVS

## Target Hardware

| Item | Detail |
|---|---|
| Board | Adafruit HUZZAH32 or ESP32-DevKitC |
| Sensor | BME280 breakout — I2C, address 0x76 or 0x77 |
| Connections | SDA → GPIO 21, SCL → GPIO 22, 3.3 V, GND |
| Average current | ~350 µA at 60 s measurement interval |

## Build & Flash

```sh
cd examples/deep-sleep-bme280-mqtt-sensor
idf.py set-target esp32
idf.py menuconfig   # set Wi-Fi SSID/password and MQTT broker URL
idf.py build flash monitor
```

## Expected Output

```
I (1842) sensor_node: BME280 → T: 23.4 °C  H: 51.2 %  P: 1013.1 hPa
I (2089) sensor_node: Wi-Fi connected in 247 ms (fast reconnect)
I (2105) sensor_node: MQTT published → sensors/office/climate (QoS 1, retained)
I (2108) sensor_node: Disconnecting and sleeping 60 s...
```

## Key Concepts

- ESP-IDF `i2c_master` driver + BME280 compensation formula implementation
- `esp_mqtt_client_publish()` with `retain=1` and `qos=1`
- `esp_wifi_set_ps(WIFI_PS_MAX_MODEM)` during active phase to cut radio current
- `esp_wifi_connect()` fast path using cached BSSID and channel in NVS

## Power Budget

| State | Current | Duration |
|---|---|---|
| Active + Wi-Fi TX | ~180 mA peak / ~90 mA avg | ~1.1 s |
| Deep sleep | ~10 µA | 58.9 s |
| **Average** | **~348 µA** | — |

## Spec Files

- [FunctionalSpec.md](FunctionalSpec.md)
- [CodingSpec.md](CodingSpec.md)
