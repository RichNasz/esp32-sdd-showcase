# Testing Specification — Deep Sleep BME280 + MQTT Sensor Node

## Testing Philosophy

Automated testing using native ESP-IDF tools is strongly preferred.
Additional tools are only allowed with explicit justification and documented benefits.
Manual testing is used only when no practical automated alternative exists.

---

## Automated Tests

### T-A1 — Zero-Warning Build

**Tool**: `idf.py` (native)

```sh
cd examples/deep-sleep-bme280-mqtt-sensor
idf.py set-target esp32s3 && idf.py build
```

Pass: exit code 0, zero compiler warnings.

---

### T-A2 — Binary Size Check

**Tool**: `idf.py` (native)

```sh
idf.py size | grep "Total binary size"
```

Pass: < 2 MB (Wi-Fi + MQTT stack included).

---

### T-A3 — Component Dependency Resolution

**Tool**: `idf.py` (native)

```sh
idf.py reconfigure 2>&1 | grep -i "error"
```

Pass: no errors; `bosch/bme280` component resolves from the ESP-IDF component registry.

---

## Manual Tests

*Hardware integration (real sensor, Wi-Fi AP, MQTT broker) cannot be replicated by simulation.*

### T-M1 — BME280 Sensor Reads Valid Data

**Why manual**: Real I2C communication and sensor calibration coefficients require physical hardware.

1. Wire BME280 breakout to XIAO ESP32S3: SDA→D4, SCL→D5, VCC→3V3, GND→GND.
2. Flash and open serial monitor.
3. Confirm log shows temperature in range 10–40 °C, humidity 10–90 %, pressure 900–1100 hPa.
4. Breathe on the sensor — confirm humidity reading increases.

Pass: all three readings in valid range; humidity responds to breath test.

---

### T-M2 — MQTT Publish Reaches Broker

**Why manual**: End-to-end MQTT delivery requires a live broker and Wi-Fi AP.

1. Start a local MQTT broker (e.g. `mosquitto`).
2. Subscribe: `mosquitto_sub -t "sdd/sensor/bme280"`.
3. Flash and monitor — confirm message arrives at broker with valid JSON.
4. Confirm `boot` counter increments on each wake.

Pass: broker receives well-formed JSON on every cycle.

---

### T-M3 — Button Wakeup

**Why manual**: Physical GPIO0 button press triggers ext1 wakeup — requires real hardware.

1. During deep sleep, press and release the BOOT button (GPIO 0).
2. Confirm immediate wakeup and serial log shows `ext1` wakeup cause.
3. Confirm a full sense+publish cycle completes before returning to sleep.

Pass: button triggers wakeup and full cycle completes.

---

### T-M4 — Active Window Timeout

**Why manual**: Requires deliberately misconfiguring Wi-Fi credentials to force a timeout.

1. Set `CONFIG_WIFI_SSID` to an invalid SSID.
2. Flash and monitor.
3. Confirm device logs a timeout warning and enters deep sleep within 15 seconds — no hang.

Pass: device sleeps within 15 seconds even when network is unavailable.
