# ==================== BOARD SELECTION ====================
# Change only this section, then regenerate the project
#
# Board: seeed-xiao-esp32s3          ← ACTIVE / DEFAULT
#
# =======================================================
# Institutional Memory: shared-specs/AIGenLessonsLearned.md — read before regenerating

# Coding Specification — Deep Sleep BME280 + MQTT Sensor Node

## Deep Sleep Configuration

- Timer wakeup: `esp_sleep_enable_timer_wakeup(15 * 1000000ULL)` (15 seconds).
- Ext1 wakeup: `esp_sleep_enable_ext1_wakeup(BIT(GPIO_NUM_0), ESP_EXT1_WAKEUP_ANY_LOW)` for button.
- `#define SLEEP_SEC 15`
- `#define ACTIVE_TIMEOUT_MS 14000` — abort and sleep if publish not complete within 14 s.
- Enter sleep: `esp_deep_sleep_start()`.

## RTC Memory

- `static RTC_DATA_ATTR uint32_t boot_count = 0;` — increment first in `app_main`.

## I2C / BME280

- I2C port: I2C_NUM_0; SDA = GPIO 5 (D4); SCL = GPIO 6 (D5).
- I2C clock: 400 kHz.
- BME280 I2C address: 0x76 (SDO tied LOW) or 0x77 (SDO HIGH) — use Kconfig option `CONFIG_BME280_I2C_ADDR` defaulting to 0x76.
- Use ESP-IDF `i2c_master` driver (ESP-IDF 5.x new driver API, not legacy `i2c_*` API).
- Read BME280 using the Bosch BME280 driver via `idf_component.yml` (component: `bosch/bme280`).
- Oversample: temperature ×2, humidity ×1, pressure ×4; mode: forced (single reading then standby).
- Wait for measurement ready: poll `bme280_get_sensor_mode()` or use `vTaskDelay(pdMS_TO_TICKS(10))`.

## Wi-Fi

- Mode: WIFI_MODE_STA.
- Use `esp_wifi_connect()` + event loop; wait for `IP_EVENT_STA_GOT_IP` before proceeding.
- Timeout: 10 seconds maximum for association + DHCP; on timeout log error and enter sleep.
- Disconnect with `esp_wifi_disconnect()` + `esp_wifi_stop()` before calling `esp_deep_sleep_start()`.
- Credentials: `CONFIG_WIFI_SSID` and `CONFIG_WIFI_PASSWORD` via `Kconfig.projbuild`.

## MQTT

- Client: `esp_mqtt_client` (ESP-IDF built-in, no external component needed).
- Broker URI: `CONFIG_MQTT_BROKER_URI` via `Kconfig.projbuild` (default: `mqtt://192.168.1.100`).
- Topic: `sdd/sensor/bme280`, QoS 0, retain = false.
- Payload format (use `snprintf`):
  `{"temp":%.1f,"hum":%.1f,"pres":%.1f,"boot":%lu}`
- Publish once; do not wait for PUBACK at QoS 0.
- Disconnect: `esp_mqtt_client_disconnect()` + `esp_mqtt_client_destroy()`.

## LED Feedback

- GPIO 21, active LOW.
- Blink once (100 ms) after successful MQTT publish.
- `gpio_hold_dis(GPIO_NUM_21)` before driving; no hold needed before sleep.

## Kconfig Options (main/Kconfig.projbuild)

```
config WIFI_SSID
    string "Wi-Fi SSID"
    default "myssid"

config WIFI_PASSWORD
    string "Wi-Fi Password"
    default "mypassword"

config MQTT_BROKER_URI
    string "MQTT Broker URI"
    default "mqtt://192.168.1.100"

config BME280_I2C_ADDR
    hex "BME280 I2C Address"
    default 0x76
    range 0x76 0x77
```

## Logging

- Tag: `"sensor-node"`
- On wake: log boot count and wakeup cause.
- After BME280 read: `ESP_LOGI(TAG, "T=%.1f°C  H=%.1f%%  P=%.1fhPa", temp, hum, pres)`
- After publish: `ESP_LOGI(TAG, "Published to %s — sleeping %d s", CONFIG_MQTT_BROKER_URI, SLEEP_SEC)`
- On timeout: `ESP_LOGW(TAG, "Active window exceeded — forcing sleep")`

## Error Handling

- `ESP_ERROR_CHECK` on all init calls.
- BME280 read failure: log error, skip publish, enter sleep.
- Wi-Fi timeout: log error, skip publish, enter sleep.
- MQTT connect failure: log error, enter sleep.
- Never abort() in production sleep loop.

## USB-CDC

- `CONFIG_ESP_CONSOLE_USB_CDC=y` in sdkconfig.defaults.
- `vTaskDelay(pdMS_TO_TICKS(100))` at start of `app_main` for CDC enumeration.

## idf_component.yml

```yaml
dependencies:
  bosch/bme280: ">=1.0.0"
```

## Agent-Generated Headers

- `.c` files: `/* AGENT-GENERATED — do not edit by hand; regenerate via esp32-sdd-full-project-generator */`
- CMakeLists.txt, sdkconfig.defaults, .gitignore, Kconfig.projbuild: `# AGENT-GENERATED — do not edit by hand; regenerate via esp32-sdd-full-project-generator`
- README.md: full HTML comment block.

## File Layout

- main/main.c
- main/CMakeLists.txt
- main/Kconfig.projbuild
- CMakeLists.txt
- sdkconfig.defaults
- idf_component.yml
- .gitignore
- README.md
