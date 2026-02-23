# Secure OTA via HTTPS — Signed Production Updates

## Overview

Production OTA example. Connects to Wi-Fi, fetches a firmware binary from an HTTPS
server, verifies the server certificate, writes the new image to the OTA partition,
and reboots into it. On next boot, the firmware marks itself valid to complete the
update; failure to mark valid within a configurable window triggers automatic rollback
to the previous image. Demonstrates the full signed-OTA lifecycle required for any
production ESP32 deployment.

## Supported Boards

- Seeed XIAO ESP32S3 (default)

## Requirements

- Connect to Wi-Fi (credentials via Kconfig).
- Fetch firmware binary via HTTPS using `esp_https_ota` component.
- Validate server TLS certificate against a bundled CA certificate (PEM, embedded via `target_add_binary_data` in CMakeLists).
- Write binary to inactive OTA partition using `esp_ota_*` API.
- Verify image integrity: check `esp_image_verify()` on the written partition before rebooting.
- Reboot into new firmware with `esp_restart()`.
- On first boot after OTA: call `esp_ota_mark_app_valid_cancel_rollback()` to confirm the update.
- If the new firmware fails to mark itself valid within `CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE` timeout, bootloader rolls back to the previous partition.
- Log all stages (connecting, downloading, writing, verifying, rebooting) over serial with progress percentage.
- Blink LED (GPIO 21) slowly during download; fast-blink 3× on OTA error.
- OTA server URL: configurable via `CONFIG_OTA_SERVER_URL` (Kconfig).

## Hardware Dependencies

- Board-spec: board-specs/seeed/xiao-esp32s3.md (ESP32-S3, LED GPIO 21 active LOW)
- No external components required (Wi-Fi built-in).

## Partition Table

- Requires a custom partition table with two OTA app partitions:
  `factory` (or `ota_0`) and `ota_1`, each ≥ 2 MB.
- `partitions.csv` must be included in the project.

## Connectivity

- Wi-Fi STA mode, WPA2-Personal.
- HTTPS only — no plaintext HTTP fallback.
- Server CA certificate embedded at build time (PEM file).

## Success Criteria

- Firmware downloads and installs without error.
- Server cert is verified; connection to a server with an invalid cert is rejected.
- After OTA reboot, new firmware boots and marks itself valid.
- Rollback test: new firmware that does not call `mark_app_valid` triggers rollback to previous image on next boot.
- No data corruption in OTA partition across 3 consecutive OTA cycles.
