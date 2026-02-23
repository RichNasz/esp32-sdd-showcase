<!-- ================================================
     AGENT-GENERATED — DO NOT EDIT BY HAND
     Generated from specs/ using esp32-sdd-documentation-generator skill
     Date: 2026-02-22 | Agent: Claude Code
     ================================================ -->

# Hardware AES Secure Storage — Crypto Accelerator

> **Security foundation.** Demonstrates ESP32's on-chip AES-256 hardware engine via mbedTLS for fast, transparent encryption of sensitive data stored in NVS.

## What It Demonstrates

- ESP32 AES hardware accelerator enabled via `CONFIG_MBEDTLS_HARDWARE_AES=y`
- AES-256-CBC encryption and decryption of arbitrary payloads
- NVS encrypted partition for key and ciphertext storage
- Hardware vs. software AES throughput benchmark — typically 15–20× faster

## Target Hardware

| Item | Detail |
|---|---|
| Board | Any ESP32 (not ESP32-S2/S3 — different crypto HW topology) |
| Flash | NVS encrypted partition required (4 MB minimum) |
| No external hardware needed | All crypto is on-chip |

## Build & Flash

```sh
cd examples/hardware-aes-secure-storage
idf.py set-target esp32
idf.py build flash monitor
```

## Expected Output

```
I (450) crypto: HW AES-256-CBC encrypt 1024 B: 0.31 ms
I (452) crypto: Decrypt + verify: OK
I (453) crypto: SW AES baseline:  4.87 ms  (15.7× slower)
I (455) crypto: Ciphertext written to NVS encrypted partition
```

## Key Concepts

- `CONFIG_MBEDTLS_HARDWARE_AES=y` in `sdkconfig.defaults` — zero code change needed
- `mbedtls_aes_context` with hardware-transparent `mbedtls_aes_setkey_enc()`
- NVS encrypted partition: `nvs_flash_secure_init()` + flash encryption key in eFuse
- Constant-time MAC comparison: `mbedtls_ct_memcmp()` to prevent timing attacks

## Security Notes

- Flash encryption and NVS encryption share the same root key burned into eFuse — **this is a one-time operation**. See `shared-specs/security-baseline.md` before enabling in production.
- For production, inject the AES key via `esp_efuse_*` APIs rather than deriving it in firmware.

## Spec Files

- [FunctionalSpec.md](FunctionalSpec.md)
- [CodingSpec.md](CodingSpec.md)
