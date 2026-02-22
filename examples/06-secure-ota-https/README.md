<!-- ================================================
     AGENT-GENERATED — DO NOT EDIT BY HAND
     Generated from specs/ using esp32-sdd-documentation-generator skill
     Date: 2026-02-22 | Agent: Claude Code
     ================================================ -->

# 06 · Secure OTA via HTTPS — Signed Production Updates

> **Production OTA.** Full over-the-air update pipeline with HTTPS transport, RSA signature verification, and automatic rollback on boot failure.

## What It Demonstrates

- ESP-IDF `esp_https_ota` component with server certificate pinning
- RSA-2048 firmware signing and `esp_image_verify()` at boot
- Dual OTA partition table with factory partition and rollback support
- `esp_ota_mark_app_invalid_rollback_and_reboot()` on repeated boot failure

## Target Hardware

| Item | Detail |
|---|---|
| Board | Any ESP32 with Wi-Fi |
| Flash | 4 MB minimum (dual OTA partition table) |
| OTA server | Any HTTPS endpoint (self-signed cert or public CA) |

## Build & Flash

```sh
cd examples/06-secure-ota-https
idf.py set-target esp32
idf.py menuconfig   # set Wi-Fi credentials and OTA server URL
idf.py build flash monitor
```

## Expected Output

```
I (2100) ota: Running firmware v1.0.0 — checking https://ota.example.com/firmware.bin
I (4820) ota: Downloading... 312 KB at 76 KB/s
I (8440) ota: Signature verified (RSA-2048, SHA-256)
I (8443) ota: OTA slot written — marking valid and rebooting
```

## Key Concepts

- `esp_https_ota_config_t` with `server_cert_pem` for certificate pinning
- `esp_ota_get_running_partition()` → `esp_ota_get_next_update_partition()`
- `esp_ota_mark_app_valid_cancel_rollback()` called only after successful self-test
- Signing workflow: `espsecure.py sign_data --keyfile signing_key.pem firmware.bin`

## Partition Table

```
# partitions.csv
nvs,       data, nvs,      0x9000,   0x6000
otadata,   data, ota,      0xf000,   0x2000
ota_0,     app,  ota_0,    0x20000,  0x180000
ota_1,     app,  ota_1,    0x1A0000, 0x180000
```

## Spec Files

- [FunctionalSpec.md](FunctionalSpec.md)
- [CodingSpec.md](CodingSpec.md)
