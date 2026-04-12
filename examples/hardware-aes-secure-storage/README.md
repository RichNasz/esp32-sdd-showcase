<!-- ================================================
     AGENT-GENERATED — DO NOT EDIT BY HAND
     Generated from specs/ using esp32-sdd-full-project-generator skill
     Date: 2026-04-12 | Agent: Claude Code
     ================================================ -->

# Hardware AES Secure Storage — Crypto Accelerator

> **Security foundation.** Demonstrates the on-chip AES-256 hardware engine accessed through mbedTLS, with a clean `secure_write` / `secure_read` API as a template for production secure storage.

## Overview

On startup, the firmware generates a fresh AES-256 key (if none exists), stores it in NVS,
then runs a round-trip self-test: encrypts a known plaintext, stores the ciphertext + IV in
NVS, retrieves it, decrypts, and verifies the result matches the original. A throughput
benchmark logs hardware AES speed (MB/s). The LED blinks once on pass (green on WS2812 boards, on-level on GPIO boards), fast-blinks
three times on failure (red / off-then-on). The example supports six boards across four SoC
targets using the same primary/secondary pattern as the blinky example.

> **Security note**: This example demonstrates the hardware crypto API, not full key lifecycle
> management. The AES key is stored in NVS in plaintext. Production systems should use ESP32
> flash encryption for key protection.

## Supported Boards

### Primary boards (auto-configured)

| Board | SoC | `idf.py set-target` | LED GPIO | LED type |
|---|---|---|---|---|
| YEJMKJ ESP32-S3-DevKitC-1-N16R8 | ESP32-S3 | `esp32s3` | GPIO 48 | WS2812 RGB |
| Espressif ESP32-C6-DevKitC-1-N8 | ESP32-C6 | `esp32c6` | GPIO 8 | WS2812 RGB |
| Seeed XIAO ESP32-C5 | ESP32-C5 | `esp32c5` | GPIO 27 | GPIO, active LOW |
| Adafruit HUZZAH32 | ESP32 | `esp32` | GPIO 13 | GPIO, active HIGH |

Primary boards are fully configured by `sdkconfig.defaults.<target>` — no menuconfig step needed.

### Secondary boards (require menuconfig after set-target)

| Board | SoC | `idf.py set-target` | LED GPIO | LED type | menuconfig overrides |
|---|---|---|---|---|---|
| Seeed XIAO ESP32S3 | ESP32-S3 | `esp32s3` | GPIO 21 | GPIO, active LOW | `EXAMPLE_LED_WS2812=n`, `EXAMPLE_LED_GPIO=21`, console → USB CDC |
| Seeed XIAO ESP32-C6 | ESP32-C6 | `esp32c6` | GPIO 15 | GPIO, active LOW | `EXAMPLE_LED_WS2812=n`, `EXAMPLE_LED_GPIO=15` |

Secondary boards share a SoC target with their primary counterpart. After `idf.py set-target`,
run `idf.py menuconfig` → **Example Configuration** → disable **Use WS2812 RGB LED** and set
**LED GPIO number** to the correct pin. For the XIAO ESP32S3 also switch the console under
**Component config → ESP System Settings** to `USB CDC`.

This primary/secondary assignment mirrors the blinky example: DevKitC boards are primary for
their SoC targets; XIAO boards are secondary when they share a target with a DevKitC.

## Prerequisites

- ESP-IDF v5.5.3 (see `shared-specs/esp-idf-version.md`)
- Any of the six supported boards listed above
- USB-C data cable

## Build & Flash

```sh
cd examples/hardware-aes-secure-storage

# Select target (default: esp32s3 / Seeed XIAO ESP32S3)
idf.py set-target esp32s3

idf.py build flash monitor
```

ESP-IDF automatically merges `sdkconfig.defaults` with the matching
`sdkconfig.defaults.<target>` when you run `idf.py set-target`, so LED GPIO and console
configuration are set correctly for primary boards without any manual editing.

### Opening in VS Code / Cursor

This folder is a complete, standalone ESP-IDF project. Open **only this folder**
(not the repository root) in VS Code or Cursor so the official ESP-IDF Extension
can detect and configure the project automatically.

**File → Open Folder… → select `examples/hardware-aes-secure-storage/`**

This gives you one-click Build / Flash / Monitor / Debug, Menuconfig, and size
analysis from the ESP-IDF commands palette. Do not open the top-level repository
folder as your active workspace while developing this example.

## Expected Serial Output

**First boot** (key is generated):

```
I (312) secure_storage: AES key generated and saved to NVS
I (315) aes_demo: Self-test: encrypting 'Hello, AES-256-CBC hardware!'
I (320) aes_demo: Self-test: PASS — plaintext matches
I (322) aes_demo: Result: PASS
I (824) aes_demo: Benchmark: encrypting 64 KB with hardware AES-256-CBC
I (836) aes_demo: Benchmark: 5.33 MB/s
I (836) aes_demo:   (hardware AES >= 5 MB/s on Xtensa; >= 2 MB/s on RISC-V)
I (836) aes_demo: Done. See specs/FunctionalSpec.md for security notes.
```

**Subsequent boots** (key loaded from NVS):

```
I (312) secure_storage: AES key loaded from NVS
I (318) aes_demo: Self-test: PASS — plaintext matches
I (318) aes_demo: Result: PASS
```

## Key Concepts

- `CONFIG_MBEDTLS_HARDWARE_AES=y` — enables the hardware AES accelerator transparently
  through the mbedTLS API; no call-site changes needed
- AES-256-CBC via `mbedtls_aes_crypt_cbc()` with a fresh random IV per write
  (`esp_fill_random()`) — IV reuse with the same key breaks CBC confidentiality
- Two-layer architecture: `secure_storage.c/.h` hides all mbedTLS and NVS internals;
  `main.c` uses only `secure_write` / `secure_read`
- mbedTLS errors (negative integers) mapped to `esp_err_t` before propagating to callers
- Multi-board LED abstraction: primary boards use `CONFIG_EXAMPLE_LED_GPIO` and
  `CONFIG_EXAMPLE_LED_ACTIVE_LEVEL` (GPIO path); secondary DevKitC boards use
  `CONFIG_EXAMPLE_LED_WS2812=y` (RMT path, green = pass, red = fail)
- Hardware AES throughput ≥ 5 MB/s on Xtensa (ESP32, ESP32-S3); ≥ 2 MB/s on RISC-V targets

## Testing

Testing philosophy: automated first, manual only when hardware observation is unavoidable.

### Automated Tests

1. **T-A1 — Zero-Warning Build**

   ```sh
   cd examples/hardware-aes-secure-storage
   idf.py set-target esp32s3 && idf.py build
   ```

   Pass: exit code 0, zero compiler warnings.

2. **T-A2 — Binary Size Check**

   ```sh
   idf.py size | grep "Total binary size"
   ```

   Pass: < 1.5 MB.

3. **T-A3 — Self-Test via Wokwi Simulation**

   Justification: mbedTLS AES-CBC runs identically in simulation; the round-trip self-test
   result is fully verifiable from serial output.

   Prerequisites: [Install Wokwi CLI](https://docs.wokwi.com/wokwi-ci/getting-started);
   `wokwi.toml` + `diagram.json` at example root.

   ```sh
   wokwi-cli --timeout 15000 --expect "Self-test: PASS" .
   ```

   Pass: `Self-test: PASS` appears in simulated serial output within 15 seconds.

4. **T-A4 — Hardware AES Enabled Confirmation**

   ```sh
   grep "CONFIG_MBEDTLS_HARDWARE_AES=y" examples/hardware-aes-secure-storage/sdkconfig.defaults
   ```

   Pass: line present (hardware accelerator correctly configured).

### Manual Tests — hardware required

**T-M1 — Self-Test on Real Hardware**

Why manual: confirms the hardware AES accelerator is invoked (not software fallback) on
actual ESP32-S3 silicon; Wokwi may use a software implementation.

1. Flash to XIAO ESP32S3: `idf.py set-target esp32s3 && idf.py build flash monitor`
2. Confirm serial shows `Self-test: PASS` and the LED blinks once (orange).
3. Confirm throughput log shows ≥ 5 MB/s (hardware AES; software typically < 1 MB/s).

Pass: PASS logged, single LED blink, throughput ≥ 5 MB/s.

---

**T-M2 — NVS Persistence Across Resets**

Why manual: NVS flash persistence requires physical flash write/read cycles.

1. Flash and run — confirm log shows "generated and saved to NVS" for the AES key.
2. Press the EN (reset) button.
3. Confirm serial shows "loaded from NVS" (key persists across reset).
4. Confirm self-test still passes (decryption works with the persisted key).

Pass: key loads from NVS after reset; self-test passes.

## Spec Files

- [specs/FunctionalSpec.md](specs/FunctionalSpec.md)
- [specs/CodingSpec.md](specs/CodingSpec.md)
- [specs/TestSpec.md](specs/TestSpec.md)
