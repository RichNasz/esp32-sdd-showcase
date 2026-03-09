# ==================== BOARD SELECTION ====================
# Change only this section, then regenerate the project
#
# Board: seeed-xiao-esp32s3          ← ACTIVE / DEFAULT
#
# =======================================================
# Institutional Memory: shared-specs/AIGenLessonsLearned.md — read before regenerating

# Coding Specification — Hardware AES Secure Storage

## Architecture

Two-layer design. A reusable secure_storage module (separate .c/.h pair) encapsulates all
AES crypto and NVS operations behind a clean read/write API. App_main uses this module to
run a round-trip self-test and a throughput benchmark — it never calls mbedTLS or NVS
directly. This separation keeps the demo concise and makes the storage API reusable.

## Key Constraints

- Algorithm: AES-256-CBC via mbedTLS with hardware acceleration enabled
  (CONFIG_MBEDTLS_HARDWARE_AES=y). The hardware accelerator is the primary point of this
  example; the sdkconfig must not omit this setting.
- IV: a fresh random IV must be generated for every write operation. The IV is stored
  prepended to the ciphertext in NVS. IV reuse with the same key breaks CBC confidentiality.
- Key: generated once on first boot from a CSPRNG, stored in NVS. The module must detect
  whether a key already exists and load it rather than regenerate.
- Padding: plaintext must be padded to the AES block boundary before encryption. The module
  must handle inputs of any length correctly.
- Benchmark: encrypt 64 KB of random data and log throughput in MB/s. Hardware AES on
  ESP32-S3 should achieve at least 5× the throughput of a software-only build.

## Preferred Libraries and APIs

Use mbedTLS (ESP-IDF built-in) for all AES operations. No external crypto library is needed.
Use NVS for key and blob storage. The secure_storage module should return esp_err_t codes
so callers never need to understand mbedTLS return values.

## Non-Functional Requirements

- The API must return meaningful error codes (key not found, buffer too small) without
  exposing crypto internals to the caller.
- mbedTLS functions return integers, not esp_err_t. All mbedTLS errors must be caught,
  logged, and mapped to appropriate esp_err_t values before propagating.
- Self-test failure must be reported visually (LED pattern) and over serial, but must not
  abort the device. An example should not crash on a test bench.
- The README must explicitly note that this example demonstrates the hardware crypto API,
  not production key lifecycle management. Production systems should use flash encryption,
  not plaintext NVS key storage.

## Gotchas

- IV uniqueness is a hard security requirement for AES-CBC. Never reuse an IV with the
  same key, even across reboots. Generating a fresh IV per write from a CSPRNG is correct.
- mbedTLS error codes are negative integers and are not compatible with ESP-IDF error
  logging macros. Use mbedtls_strerror() to produce human-readable error descriptions.
- Hardware AES operates transparently through the mbedTLS API when
  CONFIG_MBEDTLS_HARDWARE_AES=y. The API call sites do not change — only the sdkconfig
  enables the accelerator.

## File Layout (non-standard files)

- main/secure_storage.c — AES + NVS implementation
- main/secure_storage.h — public API header
