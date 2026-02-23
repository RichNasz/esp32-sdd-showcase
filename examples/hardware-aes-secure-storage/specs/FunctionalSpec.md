# Hardware AES Secure Storage — Crypto Accelerator

## Overview

Security foundation example. Demonstrates the ESP32-S3 on-chip AES-256 hardware engine
accessed through mbedTLS. Encrypts a plaintext payload, stores the ciphertext in NVS,
retrieves and decrypts it, then verifies the round-trip. Provides a clean
`secure_write` / `secure_read` API as a template for production secure storage.
A throughput benchmark logs hardware vs software AES speed.

## Supported Boards

- Seeed XIAO ESP32S3 (default)

## Requirements

- Encrypt and decrypt arbitrary byte payloads using AES-256-CBC via the ESP32-S3 hardware accelerator.
- Store encrypted blobs (ciphertext + IV) in NVS under configurable namespace and key names.
- Expose two API functions:
  - `esp_err_t secure_write(const char *key, const void *data, size_t len)`
  - `esp_err_t secure_read(const char *key, void *data, size_t *len)`
- Run a round-trip self-test on startup: encrypt a known string, store it, retrieve it, decrypt it, verify it matches the original.
- Run a throughput benchmark: encrypt 64 KB of data with hardware AES and log MB/s.
- Log all results over serial; LED blinks green (1×) on pass, fast-blinks (3×) on self-test failure.
- AES key: derived from a fixed key stored in NVS on first boot (generated via `esp_fill_random()`).

## Hardware Dependencies

- Board-spec: board-specs/seeed/xiao-esp32s3.md (ESP32-S3, LED GPIO 21 active LOW)
- No external components required.

## Security Model

- AES key is generated once on first boot, stored in NVS (plaintext for this example).
- This example demonstrates the hardware crypto API, not full key lifecycle management.
- Production systems should use flash encryption or eFuse key provisioning — noted in README.

## Success Criteria

- Round-trip self-test passes: decrypted plaintext matches original.
- Hardware AES throughput ≥ 5 MB/s (software AES typically < 1 MB/s on same core).
- Encrypted blob in NVS is not human-readable (ciphertext differs from plaintext).
- No memory leaks (heap usage before and after round-trip must be equal).
