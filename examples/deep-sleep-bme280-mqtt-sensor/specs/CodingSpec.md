# ==================== BOARD SELECTION ====================
# Change only this section, then regenerate the project
#
# Board: seeed-xiao-esp32s3          ← ACTIVE / DEFAULT
#
# =======================================================
# Institutional Memory: shared-specs/AIGenLessonsLearned.md — read before regenerating

# Coding Specification — Deep Sleep BME280 + MQTT Sensor Node

## Architecture

Linear pipeline per wake cycle: read sensor → connect Wi-Fi → publish MQTT → disconnect →
sleep. Each stage has an independent timeout. Failure at any stage skips all subsequent
stages and enters deep sleep immediately — the device must never block indefinitely waiting
for network or sensor responses. The entire pipeline runs in app_main's implicit task with
no additional FreeRTOS tasks.

## Key Constraints

- Sleep duration: 15 seconds.
- Hard active-window deadline: 15 seconds. If the full pipeline has not completed by this
  deadline, the device must abandon the current cycle and enter sleep.
- I2C bus: D4 (SDA) and D5 (SCL) at 400 kHz. Use the ESP-IDF 5.x i2c_master new driver
  API — not the legacy i2c_driver_install / i2c_master_cmd_begin pattern.
- BME280 mode: forced (single measurement, then standby). This minimises I2C active time
  and avoids running the sensor's internal ODR during the sleep period.
- Wi-Fi association timeout: 10 seconds maximum before the stage is abandoned.
- MQTT: QoS 0, retain false, single publish per cycle. QoS 1 would require PUBACK handling
  which is unnecessary complexity for this duty-cycled pattern.
- JSON payload must include temperature, humidity, pressure, and the boot counter.
- All configurable parameters (Wi-Fi credentials, broker URI, BME280 I2C address) must be
  exposed via Kconfig, not hardcoded.

## Preferred Libraries and APIs

- BME280: use the Bosch reference driver via the ESP-IDF Component Registry (idf_component.yml
  dependency). Do not write a bare I2C driver from scratch — the reference driver handles
  calibration compensation correctly.
- MQTT: use the ESP-IDF built-in esp_mqtt_client. No external library is needed.
- Wi-Fi: standard STA mode with event loop. No roaming or complex reconnect logic required.

## Non-Functional Requirements

- Wi-Fi and MQTT must be disconnected cleanly before entering deep sleep. Leaving TCP state
  open causes delays and potential failures on the next wake.
- GPIO 0 (boot button) must be configured as an ext1 wakeup source for on-demand readings.
  Its wakeup cause should produce the same pipeline as a timer wakeup.
- LED blink after successful publish is the only visual feedback. No blink on failure — the
  absence of a blink signals the operator to check the serial log.
- The example must work with a minimal MQTT broker (no TLS, no auth) for ease of setup.
  TLS is covered in the secure-ota-https example.

## Gotchas

- ADC2 is unavailable when Wi-Fi is active on ESP32-S3. All user pads on the XIAO use
  ADC1 so this does not affect this example, but the README should note it.
- The new i2c_master API (ESP-IDF 5.x) is substantially different from the legacy API.
  Using the legacy API will cause deprecation warnings or link errors depending on ESP-IDF
  version. Always use the new API.
- USB-CDC requires a brief startup delay before the first log line, specific to the XIAO's
  native USB implementation.

## File Layout (non-standard files)

- main/Kconfig.projbuild — Wi-Fi, broker URI, BME280 address configuration
- idf_component.yml — bosch/bme280 component registry dependency
