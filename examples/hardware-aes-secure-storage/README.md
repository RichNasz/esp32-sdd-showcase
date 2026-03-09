<!-- ================================================
     AGENT-GENERATED — DO NOT EDIT BY HAND
     Generated from specs/ using esp32-sdd-full-project-generator skill
     Date: 2026-03-09 | Agent: Claude Code
     ================================================ -->

# Hardware AES Secure Storage — Crypto Accelerator

> **Security foundation.** Demonstrates the on-chip AES-256 hardware engine, accessed
> through mbedTLS, for fast and transparent encryption of data stored in NVS. Provides
> a reusable `secure_write` / `secure_read` API and a throughput benchmark that confirms
> hardware acceleration is active.

> **Security disclaimer**: This example demonstrates the hardware crypto API, not
> production key lifecycle management. The AES key is generated once on first boot and
> stored in NVS as plaintext. Production systems must use flash encryption for key
> protection. See `shared-specs/security-baseline.md`.

---

## Overview

On startup the example:

1. Initialises NVS and the `secure_storage` module (generates an AES-256 key on first boot, or loads it from NVS on subsequent boots).
2. Runs a **round-trip self-test** — encrypts a known string, stores the ciphertext in NVS, retrieves and decrypts it, then asserts the result matches the original plaintext.
3. Blinks the onboard LED once (pass) or three times rapidly (failure).
4. Runs a **64 KB throughput benchmark** and logs MB/s — hardware AES should be at least 5× faster than a software-only build on Xtensa targets.

All logic that touches mbedTLS or NVS is isolated in `main/secure_storage.c` behind the `secure_write` / `secure_read` API, making the module straightforward to copy into a production project.

---

## Prerequisites

| Item | Detail |
| --- | --- |
| ESP-IDF | v5.3.0 or later (v5.4.x recommended) |
| Supported targets | ESP32, ESP32-S3, ESP32-C5, ESP32-C6 |
| Hardware | One of the supported boards (see table below) |
| External components | None — all crypto is on-chip |

### Supported Boards

| Board | Target | LED GPIO | LED polarity |
| --- | --- | --- | --- |
| Seeed XIAO ESP32S3 (default) | `esp32s3` | GPIO 21 | Active LOW (orange) |
| Seeed XIAO ESP32-C6 | `esp32c6` | GPIO 15 | Active LOW (blue) |
| Seeed XIAO ESP32-C5 | `esp32c5` | GPIO 27 | Active LOW (yellow) |
| Adafruit HUZZAH32 | `esp32` | GPIO 13 | Active HIGH (red) |

---

## Build & Flash

```sh
cd examples/hardware-aes-secure-storage
```

**Seeed XIAO ESP32S3** (default):
```sh
idf.py set-target esp32s3
idf.py build flash monitor
```

**Seeed XIAO ESP32-C6**:
```sh
idf.py set-target esp32c6
idf.py build flash monitor
```

**Seeed XIAO ESP32-C5**:
```sh
idf.py set-target esp32c5
idf.py build flash monitor
```

**Adafruit HUZZAH32**:
```sh
idf.py set-target esp32
idf.py build flash monitor
```

ESP-IDF automatically merges `sdkconfig.defaults` with the matching `sdkconfig.defaults.<target>` when you run `idf.py set-target`, so LED GPIO and console configuration are set correctly for each board without any manual editing.

---

## Opening in VS Code / Cursor

This folder is a complete, standalone ESP-IDF project. Open **only this folder**
(not the repository root) in VS Code or Cursor so the official ESP-IDF Extension
can detect and configure the project automatically.

**File → Open Folder… → select `examples/hardware-aes-secure-storage/`**

This gives you one-click Build / Flash / Monitor / Debug, Menuconfig, and size
analysis from the ESP-IDF commands palette. Do not open the top-level repository
folder as your active workspace while developing this example.

---

## Expected Serial Output

**First boot** (key is generated):

```
I (312) secure_storage: AES key not found; generating and saving to NVS
I (318) secure_storage: AES key generated and saved to NVS
I (320) aes_demo: Self-test: encrypting 'Hello, AES-256-CBC hardware!'
I (325) aes_demo: Self-test: reading from NVS and decrypting
I (330) aes_demo: Self-test: PASS — plaintext matches
I (332) aes_demo: Result: PASS
I (834) aes_demo: Benchmark: encrypting 64 KB with hardware AES-256-CBC
I (836) aes_demo: Benchmark: 12.47 MB/s
I (836) aes_demo:   (hardware AES >= 5 MB/s on Xtensa; >= 2 MB/s on RISC-V)
I (837) aes_demo: Done. See specs/FunctionalSpec.md for security notes.
```

**Subsequent boots** (key is loaded from NVS):

```
I (312) secure_storage: AES key loaded from NVS
I (318) aes_demo: Self-test: PASS — plaintext matches
...
```

---

## Key Concepts

- **`CONFIG_MBEDTLS_HARDWARE_AES=y`** in `sdkconfig.defaults` — enables the hardware AES accelerator transparently through the mbedTLS API; no call-site changes needed.
- **AES-256-CBC** via `mbedtls_aes_crypt_cbc()` — the hardware accelerator is invoked automatically when the Kconfig option is set.
- **Fresh IV per write** — `esp_fill_random()` generates a new 16-byte IV for every `secure_write()` call. IV reuse with the same key would break CBC confidentiality; this pattern is the correct mitigation.
- **PKCS7 padding** — plaintext is padded to the AES block boundary before encryption and validated and stripped after decryption.
- **NVS blob storage** — encrypted blobs (IV + ciphertext) are stored and retrieved with `nvs_set_blob()` / `nvs_get_blob()` under a configurable key name.
- **Multi-board LED abstraction** — `CONFIG_EXAMPLE_LED_GPIO` and `CONFIG_EXAMPLE_LED_ACTIVE_LEVEL` (set via per-target `sdkconfig.defaults.<target>`) eliminate `#if CONFIG_IDF_TARGET_*` guards in application code.
- **`esp_timer_get_time()`** — microsecond-resolution timer used for the throughput benchmark.

---

## Testing

### Automated Tests

**T-A1 — Zero-Warning Build**

```sh
cd examples/hardware-aes-secure-storage
idf.py set-target esp32s3 && idf.py build
```

Pass: exit code 0, zero compiler warnings.

**T-A2 — Binary Size Check**

```sh
idf.py size | grep "Total binary size"
```

Pass: total binary size < 1.5 MB.

**T-A3 — Self-Test via Wokwi Simulation**

> Prerequisite: Wokwi CLI installed; `wokwi.toml` and `diagram.json` placed at the example root (not generated — see [Wokwi docs](https://docs.wokwi.com/wokwi-ci/getting-started) for setup). mbedTLS AES-CBC runs identically in simulation so the round-trip result is fully verifiable from serial output.

```sh
wokwi-cli --timeout 15000 --expect "Self-test: PASS" .
```

Pass: `Self-test: PASS` appears in simulated serial output within 15 seconds.

**T-A4 — Hardware AES Config Confirmation**

```sh
grep "CONFIG_MBEDTLS_HARDWARE_AES=y" sdkconfig.defaults
```

Pass: line present (confirms hardware accelerator is configured).

### Manual Tests — Hardware Required

**T-M1 — Self-Test on Real Silicon**

Why manual: confirms the hardware AES accelerator is invoked on actual silicon; Wokwi may use a software fallback.

1. Flash to the target board (example shown for XIAO ESP32S3):
   ```sh
   idf.py set-target esp32s3 && idf.py build flash
   ```
2. Open the serial monitor:
   ```sh
   idf.py monitor
   ```
3. Confirm serial shows `Self-test: PASS` and the LED blinks once slowly.
4. Confirm the throughput log shows ≥ 5 MB/s (Xtensa) or ≥ 2 MB/s (RISC-V). Software AES typically measures < 1 MB/s on the same core.

Pass: PASS logged, single LED blink, throughput above threshold.

**T-M2 — NVS Persistence Across Resets**

Why manual: NVS flash persistence requires physical write/read cycles through actual flash.

1. Flash and run — confirm the serial log shows `AES key generated and saved to NVS` (first boot).
2. Press the reset / EN button.
3. Confirm the serial log shows `AES key loaded from NVS` (key persists).
4. Confirm `Self-test: PASS` still appears (decryption works with the persisted key).

Pass: key loads from NVS after reset; self-test passes.

---

## Spec Files

- [FunctionalSpec.md](specs/FunctionalSpec.md)
- [CodingSpec.md](specs/CodingSpec.md)
- [TestSpec.md](specs/TestSpec.md)
