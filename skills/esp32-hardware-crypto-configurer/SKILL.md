---
name: esp32-hardware-crypto-configurer
description: Configures ESP32 hardware security subsystems. Generates Kconfig, sdkconfig fragments, and C implementation stubs for AES/SHA/RSA accelerators, NVS encryption, flash encryption, secure boot, and eFuse provisioning workflows.
version: "1.0.0"
author: Richard Naszcyniec
keywords: sdd, esp32, security, aes, sha, rsa, secure-boot, flash-encryption, nvs, efuse, crypto
---

<!-- ================================================
     AGENT-GENERATED — DO NOT EDIT BY HAND
     Generated from specs/ using esp32-sdd-documentation-generator skill
     Date: 2026-02-22 | Agent: Claude Code
     ================================================ -->

# ESP32 Hardware Crypto Configurer

## When to use
When a FunctionalSpec requires encryption, authenticated storage, OTA signature verification, or secure boot. Invoked automatically as a sub-workflow by `esp32-sdd-full-project-generator` when security requirements are present.

## Workflow (follow exactly)
1. Read the FunctionalSpec.md security requirements section.
2. Select the minimal set of hardware accelerators needed (AES-128/256, SHA-256/512, RSA-2048, ECDSA) and document the rationale.
3. Generate `components/crypto/crypto_init.c` and `crypto_init.h` with mbedTLS hardware acceleration hooks (`MBEDTLS_AES_ALT`, `MBEDTLS_SHA256_ALT`, etc.).
4. Generate NVS encryption key partition entry and `nvs_flash_secure_init()` usage if persistent encrypted storage is required.
5. Generate flash encryption `sdkconfig.defaults` fragment (development vs. release mode) with clear warnings about one-time eFuse burn implications.
6. Generate secure boot V2 signing workflow: key generation, `espsecure.py sign_data`, and `bootloader_config.h` overrides.
7. Emit eFuse provisioning script stubs (`espefuse.py`) for production key injection workflows.
8. Document all security decisions, trade-offs, and irreversible operations in the example README security section.
