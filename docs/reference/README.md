<!-- ================================================
     AGENT-GENERATED — DO NOT EDIT BY HAND
     Generated from specs/ using esp32-sdd-documentation-generator skill
     Date: 2026-03-08 | Agent: Claude Code
     ================================================ -->

# Reference

Quick-reference documentation for the ESP32 SDD Showcase — API summaries, configuration options, pin tables, and spec format references.

## Available References

| Reference | Description |
|---|---|
| [sdkconfig-options.md](sdkconfig-options.md) | Index of all `CONFIG_*` options set across the 8 examples with rationale |
| [pin-mapping-tables.md](pin-mapping-tables.md) | GPIO → peripheral tables for every supported board |
| [deep-sleep-api.md](deep-sleep-api.md) | ESP-IDF deep sleep API quick reference with wakeup source comparison |
| [crypto-api.md](crypto-api.md) | mbedTLS hardware acceleration hooks and NVS encryption API summary |
| [esp-now-api.md](esp-now-api.md) | ESP-NOW init, peer management, and send/receive callback reference |
| [ota-api.md](ota-api.md) | `esp_https_ota` component API and partition table quick reference |
| [spec-format.md](spec-format.md) | Required sections and field definitions for `FunctionalSpec.md` and `CodingSpec.md` |
| [skill-invocation.md](skill-invocation.md) | Canonical invocation phrases and argument patterns for all 6 skills |

## ESP-IDF Version

All reference material targets the ESP-IDF version pinned in `shared-specs/esp-idf-version.md`. API signatures may differ in other versions.

## Chip Variant Notes

Several features differ across ESP32 family members:

| Feature | ESP32 | ESP32-S2 | ESP32-S3 | ESP32-C3 | ESP32-C5 | ESP32-C6 |
|---|---|---|---|---|---|---|
| Hardware AES | Yes | Yes | Yes | Yes | Yes | Yes |
| Hardware RSA | Yes | Yes | Yes | No | No | No |
| BLE 5.0 | BLE 4.2 | No BLE | BLE 5.0 | BLE 5.0 | BLE 5.3 | BLE 5.3 |
| Touch pads | 10 | 14 | 14 | No | No | No |
| ULP | FSM | FSM | RISC-V | No | No | No |

Consult the relevant board spec in `board-specs/` for your exact module.
