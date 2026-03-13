# ==================== BOARD SELECTION ====================
# Change only this section, then regenerate the project
#
# Board: YEJMKJ ESP32-S3-DevKitC-1-N16R8          ← ACTIVE / DEFAULT
#
# =======================================================
# Institutional Memory: shared-specs/AIGenLessonsLearned.md — read before regenerating

# Coding Specification — Secure OTA via HTTPS

## Architecture

Linear OTA pipeline. App_main performs Wi-Fi association, immediately checks for a pending
rollback and validates the running image if needed, then initiates the HTTPS OTA download.
A separate FreeRTOS task drives the LED blink during the download so app_main can block on the
transfer without losing visual feedback. On success, app_main reboots. On any failure, the
pipeline halts with a visible error LED pattern and does not reboot silently.

## Key Constraints

- Server certificate is embedded at build time from main/server_cert.pem using
  target_add_binary_data in CMakeLists. This file is a placeholder — operators must replace it
  with their actual CA certificate before use.
- Wi-Fi association timeout: 30 seconds before the pipeline is abandoned.
- HTTPS client timeout: 30 seconds to avoid indefinite blocking on a stalled server connection.
- Partition table: dual OTA layout (ota_0 + ota_1 + otadata) defined in a custom partitions.csv
  at the example root. The default ESP-IDF partition table is too small for two OTA app slots.
- Rollback: CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=y must be set. The newly-booted image must
  call esp_ota_mark_app_valid_cancel_rollback() early in app_main — before any user logic that
  could fail. Failure to do so causes the bootloader to roll back on the next reboot.
- All configurable parameters (Wi-Fi credentials, OTA server URL) must be exposed via Kconfig,
  not hardcoded.

## Preferred Libraries and APIs

Use esp_https_ota for the OTA download. It handles chunked writes to the OTA flash partition,
HTTPS negotiation, and progress callbacks in a single API call. Implementing this manually via
esp_ota_write would duplicate logic that esp_https_ota already handles correctly.

Use standard STA Wi-Fi with event loop for network connectivity — the same pattern as the
deep-sleep-bme280-mqtt-sensor example. No reconnect or roaming logic is needed.

## Hardware Interface

| Signal         | GPIO | Direction | Notes                                      |
|----------------|------|-----------|--------------------------------------------|
| LED Left       | 4    | Output    | Active-high, 3.3 V drive; steady ON = ota_0 active |
| LED Middle     | 5    | Output    | Active-high; blinks during OTA, error pattern on failure |
| LED Right      | 6    | Output    | Active-high; steady ON = ota_1 active      |
| Button Trigger | 7    | Input     | Active-low; ESP32-S3 internal pull-up enabled |

## Non-Functional Requirements

- OTA API failures must not trigger abort(). Log the error with esp_err_to_name, blink the
  error LED pattern, and then loop without rebooting. Silent reboots on persistent OTA failure
  would create an infinite retry loop if the server is unavailable.
- Wi-Fi connection timeout must log a clear message and halt execution — do not reboot silently.
  The operator must be able to observe the failure cause from the serial log.
- The LED blink during download must run on a separate FreeRTOS task, because app_main blocks
  on the esp_https_ota call for the full transfer duration.
- All state transitions must be logged via ESP_LOGx: Wi-Fi connect/fail, OTA start/progress
  percentage/success/fail, and rollback-check result. This is mandatory for field diagnostics
  over the native USB serial console.

## Gotchas

- If rollback is enabled but esp_ota_mark_app_valid_cancel_rollback() is never called, the
  device will roll back on every subsequent reboot even after a successful OTA. This call must
  be unconditional and must happen before any user logic.
- The PEM file embedded in the binary must match the CA that signed the OTA server's TLS
  certificate. For a self-signed server, embed the server's own certificate — not a root CA.
- Custom partition tables require CONFIG_PARTITION_TABLE_CUSTOM=y and
  CONFIG_PARTITION_TABLE_CUSTOM_FILENAME pointing to the CSV in sdkconfig.defaults.
- The YEJMKJ board uses native USB only (no CH343P bridge). Its single USB-C connector
  exposes two logical interfaces: a JTAG/DFU interface for flashing and a USB Serial/JTAG
  CDC interface for console output. Flash via the usbmodem JTAG port, monitor via the
  usbmodem CDC port. Set CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y in sdkconfig.defaults.
  Do NOT set CONFIG_ESP_CONSOLE_USB_CDC=y.

## File Layout (non-standard files)

- main/Kconfig.projbuild — Wi-Fi SSID/password, OTA server URL
- main/server_cert.pem — placeholder PEM; operator replaces with real CA cert
- partitions.csv — dual OTA partition table at example root
