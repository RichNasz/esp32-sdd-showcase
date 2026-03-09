# Hardware AES Secure Storage — Crypto Accelerator

## Overview

Security foundation example. Demonstrates the on-chip AES-256 hardware engine accessed
through mbedTLS. Encrypts a plaintext payload, stores the ciphertext in NVS, retrieves
and decrypts it, then verifies the round-trip. Provides a clean `secure_write` /
`secure_read` API as a template for production secure storage. A throughput benchmark
logs hardware vs software AES speed.

## Supported Boards

| Board | SoC | idf.py target | LED GPIO | LED polarity | Console config |
| --- | --- | --- | --- | --- | --- |
| Seeed XIAO ESP32S3 (default) | ESP32-S3 | `esp32s3` | GPIO 21 | Active LOW | `CONFIG_ESP_CONSOLE_USB_CDC=y` |
| Seeed XIAO ESP32-C6 | ESP32-C6 | `esp32c6` | GPIO 15 | Active LOW | `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y` |
| Seeed XIAO ESP32-C5 | ESP32-C5 | `esp32c5` | GPIO 27 | Active LOW | `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y` |
| Adafruit HUZZAH32 | ESP32 | `esp32` | GPIO 13 | Active HIGH | UART (CP2104; no extra config needed) |

Multi-board guard: wrap board-specific GPIO and console config in `#if CONFIG_IDF_TARGET_*` blocks.

## Requirements

- Encrypt and decrypt arbitrary byte payloads using AES-256-CBC via the SoC hardware accelerator.
- Store encrypted blobs (ciphertext + IV) in NVS under configurable namespace and key names.
- Expose two API functions:
  - `esp_err_t secure_write(const char *key, const void *data, size_t len)`
  - `esp_err_t secure_read(const char *key, void *data, size_t *len)`
- Run a round-trip self-test on startup: encrypt a known string, store it, retrieve it, decrypt it, verify it matches the original.
- Run a throughput benchmark: encrypt 64 KB of data with hardware AES and log MB/s.
- Log all results over serial; LED blinks once on pass, fast-blinks three times on self-test failure.
- AES key: generated once on first boot via `esp_fill_random()` and stored in NVS.

## Hardware Dependencies

- Board-specs:
  - board-specs/seeed/xiao-esp32s3.md (ESP32-S3, LED GPIO 21 active LOW)
  - board-specs/seeed/xiao-esp32c6.md (ESP32-C6, LED GPIO 15 active LOW)
  - board-specs/seeed/xiao-esp32c5.md (ESP32-C5, LED GPIO 27 active LOW)
  - board-specs/adafruit/huzzah32.md (ESP32, LED GPIO 13 active HIGH)
- No external components required on any supported board.

## Security Model

- AES key is generated once on first boot, stored in NVS (plaintext for this example).
- This example demonstrates the hardware crypto API, not full key lifecycle management.
- Production systems should use flash encryption for key protection — noted in README.

## Success Criteria

- Round-trip self-test passes: decrypted plaintext matches original.
- Hardware AES throughput ≥ 5 MB/s on Xtensa targets (ESP32, ESP32-S3); ≥ 2 MB/s on RISC-V targets (ESP32-C5, ESP32-C6).
- Encrypted blob in NVS is not human-readable (ciphertext differs from plaintext).
- No memory leaks (heap usage before and after round-trip must be equal).
- Builds cleanly for all four targets without warnings.
