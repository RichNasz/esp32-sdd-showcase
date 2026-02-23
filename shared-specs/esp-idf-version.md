# ESP-IDF Version Constraints

**Classification:** Human-authored shared spec
**Applies to:** All examples in this repository

---

## Pinned ESP-IDF Version

| Constraint | Value |
|---|---|
| **ESP-IDF version** | v5.4.x (latest stable in the 5.4 line) |
| **Minimum** | v5.3.0 |
| **Maximum tested** | v5.4.1 |
| **Toolchain** | xtensa-esp-elf-gcc 13.2.0 (bundled with ESP-IDF v5.4) |
| **Python** | 3.10 or 3.11 (ESP-IDF installation requirement) |
| **CMake** | 3.24 or later |
| **Ninja** | 1.11 or later |

## Supported Target SoCs

| SoC | Status | Notes |
|---|---|---|
| ESP32 | Supported | Xtensa LX6 dual-core; target `esp32` |
| ESP32-S3 | Supported | Xtensa LX7 dual-core + USB OTG; target `esp32s3` |
| ESP32-S2 | Supported | Xtensa LX7 single-core; target `esp32s2` |
| ESP32-C3 | Supported | RISC-V single-core; target `esp32c3` |
| ESP32-C6 | Supported | RISC-V dual-core + 802.15.4; target `esp32c6` |
| ESP32-H2 | Supported | RISC-V + 802.15.4 only (no Wi-Fi); target `esp32h2` |

Each example's `FunctionalSpec.md` must declare which targets it supports. Multi-target examples must compile cleanly for every declared target.

## Version Locking Rationale

v5.4 is chosen because:

- It is the current ESP-IDF long-term support (LTS) candidate as of early 2026.
- The `esp_wifi`, `esp_netif`, `esp_event`, and `esp_timer` APIs have been stable since v5.1 and breaking changes in this range are minimal.
- `idf_component_manager` (IDF Component Manager v2) is fully supported, enabling reproducible component installs via `idf_component.yml`.
- Thread-safe NVS and `nvs_flash_init_partition` are available without workarounds.
- Hardware crypto accelerators (AES, SHA, RSA, ECC) expose a stable `esp_hw_support` interface.

## sdkconfig.defaults Philosophy

All examples pin SDK options in `sdkconfig.defaults` rather than leaving them at menuconfig defaults. This ensures reproducible builds across developer machines. Options not listed in `sdkconfig.defaults` take the upstream default.

Required entries in every `sdkconfig.defaults`:

```
CONFIG_IDF_TARGET="esp32"        # or the appropriate target
CONFIG_IDF_FIRMWARE_CHIP_ID=...  # set by idf.py set-target; do not set manually
```

## Upgrading ESP-IDF

When the project moves to a new ESP-IDF minor version:

1. Update the **Pinned ESP-IDF Version** table above.
2. Update `esp-idf-version.md` (this file) and regenerate docs.
3. Rebuild and flash every example to confirm no regressions.
4. Update `AIGenLessonsLearned.md` if any breaking API changes are encountered.

Do not upgrade mid-example-development without running the full validation suite first.
