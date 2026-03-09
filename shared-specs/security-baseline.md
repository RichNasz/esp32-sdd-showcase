# Security Baseline

**Classification:** Human-authored shared spec
**Applies to:** All examples; mandatory for any example marked `production-targeted` in its FunctionalSpec.md

---

## Threat Model Scope

This baseline addresses the threat model relevant to ESP32 firmware examples in this showcase:

- **Firmware confidentiality** — prevent extraction of proprietary algorithms or credentials stored in flash.
- **Firmware integrity** — prevent unauthorized firmware modification (physical or OTA).
- **Credential protection** — prevent extraction of Wi-Fi PSKs, API keys, TLS private keys stored in NVS.
- **Communication security** — prevent eavesdropping or tampering of OTA updates and cloud connections.

Out of scope for this baseline: physical decapping attacks, RF-layer attacks, denial-of-service, cloud backend security.

---

## Minimum Requirements (All Examples)

These apply regardless of whether an example is production-targeted.

| Requirement | Implementation |
|---|---|
| No hardcoded credentials | Wi-Fi PSK, API keys, and tokens must come from NVS or Kconfig (not `#define` in source) |
| No debug UART credential leakage | Credentials must never be printed via `ESP_LOGI` or any log level |
| Input validation at system boundaries | All data received over UART, BLE, or network must be length-checked before use |
| Stack overflow protection | `CONFIG_FREERTOS_CHECK_STACKOVERFLOW_CANARY=y` in all sdkconfig.defaults |
| Watchdog enabled | `CONFIG_ESP_TASK_WDT_EN=y`; see `coding-conventions.md` |

---

## Production-Targeted Requirements

Examples with `Production Target: yes` in their FunctionalSpec.md must also satisfy:

### Flash Encryption

```
CONFIG_SECURE_FLASH_ENC_ENABLED=y
CONFIG_SECURE_FLASH_ENCRYPTION_MODE_DEVELOPMENT=y  # dev boards only
# CONFIG_SECURE_FLASH_ENCRYPTION_MODE_RELEASE=y    # enable for production eFuse burn
```

- Flash encryption uses AES-XTS-256 on all supported targets (ESP32 uses AES-XTS-128 due to hardware).
- The encryption key is generated on-device during first boot and burned into eFuse. Never extract or store it.
- `esptool.py` flash operations require `--no-stub` and appropriate encryption flags when flash encryption is active.

### Secure Boot v2

```
CONFIG_SECURE_BOOT=y
CONFIG_SECURE_BOOT_V2_ENABLED=y
CONFIG_SECURE_BOOT_SIGNING_KEY="secure_boot_signing_key.pem"
CONFIG_SECURE_BOOT_BUILD_SIGNED_BINARIES=y
```

- RSA-PSS 3072-bit signing key. Store the private key offline (not in the repository).
- Secure Boot v2 supports key revocation; plan for at most 3 signing keys per device.
- Once Secure Boot is enabled in production, it cannot be disabled without reflashing the eFuses (irreversible).

### NVS Encryption

```
CONFIG_NVS_ENCRYPTION=y
```

- Requires flash encryption to be enabled first.
- NVS encryption key partition must be declared in the partition table (see `partition-table-profiles.md`).
- Use `nvs_sec_provider` component for key provisioning in production.

### TLS/HTTPS Requirements

- All outbound HTTP connections must use HTTPS with server certificate verification enabled.
- `esp_http_client` must be configured with `cert_pem` pointing to the CA bundle or server certificate.
- Do not use `use_global_ca_store = true` without understanding which CA bundle is embedded.
- Minimum TLS version: TLS 1.2. Disable TLS 1.0 and 1.1 via mbedTLS Kconfig.
- Certificate expiry must be handled gracefully — log and alert rather than silently failing.

### OTA Update Security

- OTA updates must be fetched over HTTPS with certificate verification.
- Verify the incoming firmware signature before writing to the OTA partition (requires Secure Boot v2 app signing).
- Always boot-verify the new partition before committing: use `esp_ota_mark_app_valid_cancel_rollback()` only after confirming connectivity.
- Keep at least one verified rollback partition available at all times.

---

## Credentials Management

| Credential Type | Storage | Notes |
|---|---|---|
| Wi-Fi PSK | NVS namespace `wifi_cfg` | Provisioned via BLE/SmartConfig on first boot; never in sdkconfig.defaults |
| API keys / tokens | NVS namespace `credentials` | Injected at provisioning; encrypted NVS if production-targeted |
| TLS private key | NVS or dedicated DS (Digital Signature) peripheral | Use DS peripheral on ESP32-S3/C3 for production; private key never leaves hardware |
| OTA signing public key | Embedded in firmware via Secure Boot config | Part of the signed binary |

---

## Common Vulnerabilities to Avoid

The full-project-generator must not emit code that exhibits these patterns:

| Vulnerability | Pattern to Avoid |
|---|---|
| Buffer overflow | Unbounded `strcpy`, `sprintf`, `memcpy` with user-supplied lengths |
| Format string | `printf(user_input)` — always use `printf("%s", user_input)` |
| Integer overflow | Unchecked arithmetic on lengths before `malloc` |
| Use after free | Accessing a handle after `esp_xxx_driver_delete()` |
| TOCTOU | Checking a flag then acting on a shared resource without a mutex |
| Hardcoded IV/nonce | Using a fixed IV with AES-CBC — always use random IV per message |

---

## Security Review Checklist

Before marking an example as production-ready, verify:

- [ ] Flash encryption enabled and key generated on-device.
- [ ] Secure Boot v2 enabled and signing key stored offline.
- [ ] NVS encryption enabled.
- [ ] All credentials stored in NVS, not in source or sdkconfig.
- [ ] TLS certificate verification enabled for all outbound connections.
- [ ] OTA update path uses HTTPS and signature verification.
- [ ] No debug logs expose credentials.
- [ ] Stack canary enabled.
- [ ] Watchdog enabled and task subscriptions confirmed.
- [ ] Input validation at all external data boundaries.
