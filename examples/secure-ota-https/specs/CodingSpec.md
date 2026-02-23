# ==================== BOARD SELECTION ====================
# Change only this section, then regenerate the project
#
# Board: seeed-xiao-esp32s3          ← ACTIVE / DEFAULT
#
# =======================================================
# Institutional Memory: shared-specs/AIGenLessonsLearned.md — read before regenerating

# Coding Specification — Secure OTA via HTTPS

## OTA Flow (app_main sequence)

1. GPIO 21 init (LED output).
2. NVS flash init: `nvs_flash_init()`.
3. Wi-Fi init and connect (wait for `IP_EVENT_STA_GOT_IP`, 30 s timeout).
4. Check if this is first boot after OTA: `esp_ota_get_running_partition()` vs `esp_ota_get_boot_partition()`.
   - If running in new OTA slot: call `esp_ota_mark_app_valid_cancel_rollback()` immediately.
5. Begin HTTPS OTA: `esp_https_ota()` with config struct.
6. On success: `esp_restart()`.
7. On failure: log error, fast-blink LED 3×, loop forever (do not silently reboot on error).

## HTTPS OTA Configuration

```c
esp_http_client_config_t http_config = {
    .url            = CONFIG_OTA_SERVER_URL,
    .cert_pem       = (char *)server_cert_pem_start,
    .timeout_ms     = 30000,
    .keep_alive_enable = true,
};
esp_https_ota_config_t ota_config = {
    .http_config = &http_config,
};
```

- Server certificate: embed `main/server_cert.pem` using `target_add_binary_data` in `main/CMakeLists.txt`.
- External symbols: `extern const uint8_t server_cert_pem_start[] asm("_binary_server_cert_pem_start");`

## Partition Table

- File: `partitions.csv` at example root.
- Content:
```csv
# Name,   Type, SubType, Offset,  Size, Flags
nvs,      data, nvs,     0x9000,  0x6000,
otadata,  data, ota,     0xf000,  0x2000,
ota_0,    app,  ota_0,   0x20000, 0x200000,
ota_1,    app,  ota_1,   0x220000,0x200000,
```
- sdkconfig.defaults: `CONFIG_PARTITION_TABLE_CUSTOM=y`, `CONFIG_PARTITION_TABLE_CUSTOM_FILENAME="partitions.csv"`.
- sdkconfig.defaults: `CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=y`.

## Wi-Fi

- Same pattern as deep-sleep-bme280-mqtt-sensor: event loop, `IP_EVENT_STA_GOT_IP`, 30 s timeout.
- Credentials via `CONFIG_WIFI_SSID` / `CONFIG_WIFI_PASSWORD` in `main/Kconfig.projbuild`.

## Kconfig Options (main/Kconfig.projbuild)

```
config WIFI_SSID
    string "Wi-Fi SSID"
    default "myssid"

config WIFI_PASSWORD
    string "Wi-Fi Password"
    default "mypassword"

config OTA_SERVER_URL
    string "OTA Firmware Server URL"
    default "https://192.168.1.100:8070/firmware.bin"
```

## LED Feedback

- GPIO 21, active LOW.
- During download: slow blink 500 ms on / 500 ms off using `vTaskDelay` in a loop.
- Run LED blink in a separate FreeRTOS task; signal task to stop via `xTaskNotify` when OTA completes or fails.
- On error: 3× fast blink (100 ms on / 100 ms off), then LED off.

## Logging

- Tag: `"ota"`
- Wi-Fi connected: `ESP_LOGI(TAG, "IP: " IPSTR, IP2STR(&event->ip_info.ip))`
- OTA start: `ESP_LOGI(TAG, "Fetching: %s", CONFIG_OTA_SERVER_URL)`
- OTA complete: `ESP_LOGI(TAG, "OTA done — rebooting")`
- Rollback cancel: `ESP_LOGI(TAG, "Marked valid — rollback cancelled")`
- Error: `ESP_LOGE(TAG, "OTA failed: %s", esp_err_to_name(err))`

## Error Handling

- `ESP_ERROR_CHECK` on all init calls.
- OTA API calls: check return value, log with `esp_err_to_name`, do not use `ESP_ERROR_CHECK` (allows graceful error LED behaviour instead of abort).
- Wi-Fi timeout: log and halt (do not reboot silently — operator must investigate).

## sdkconfig.defaults

```
CONFIG_ESP_CONSOLE_USB_CDC=y
CONFIG_PARTITION_TABLE_CUSTOM=y
CONFIG_PARTITION_TABLE_CUSTOM_FILENAME="partitions.csv"
CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=y
```

## Agent-Generated Headers

- `.c` files: `/* AGENT-GENERATED — do not edit by hand; regenerate via esp32-sdd-full-project-generator */`
- CMakeLists.txt, sdkconfig.defaults, .gitignore, Kconfig.projbuild, partitions.csv: `# AGENT-GENERATED — do not edit by hand; regenerate via esp32-sdd-full-project-generator`
- README.md: full HTML comment block.

## File Layout

- main/main.c
- main/CMakeLists.txt
- main/Kconfig.projbuild
- main/server_cert.pem  ← placeholder; operator replaces with real CA cert
- CMakeLists.txt
- partitions.csv
- sdkconfig.defaults
- .gitignore
- README.md
