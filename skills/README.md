<!-- ================================================
     AGENT-GENERATED — DO NOT EDIT BY HAND
     Generated from specs/ using esp32-sdd-documentation-generator skill
     Date: 2026-04-12 | Agent: Claude Code
     ================================================ -->

# skills/

Agent skills published for this repository. Each skill is a self-contained AI workflow definition that any Claude Code agent can execute. Together they implement the complete 100% AI-first SDD pipeline for ESP32.

## Available Skills

| Skill | Description |
|---|---|
| [esp32-sdd-functional-spec-creator](esp32-sdd-functional-spec-creator/SKILL.md) | Guides creation of a `FunctionalSpec.md` for a new example interactively. |
| [esp32-sdd-full-project-generator](esp32-sdd-full-project-generator/SKILL.md) | The primary code-gen skill — produces a complete ESP-IDF project from spec files. |
| [esp32-sdd-documentation-generator](esp32-sdd-documentation-generator/SKILL.md) | Generates ALL Markdown documentation from `specs/`. Run after any spec change. |
| [esp32-board-spec-generator](esp32-board-spec-generator/SKILL.md) | Creates structured board-spec Markdown from vendor datasheets and product pages. |
| [esp32-ws2812-led-engineer](esp32-ws2812-led-engineer/SKILL.md) | Generates the canonical WS2812B RMT driver block, Kconfig symbols, CMakeLists dependency, and per-target sdkconfig.defaults fragments. Handles primary/secondary board pattern for same-chip multi-board support. |
| [esp32-ble-beacon-engineer](esp32-ble-beacon-engineer/SKILL.md) | Designs NimBLE advertising, manufacturer data PDUs, bounded advertising windows, and BLE stack teardown before deep sleep. |
| [esp32-deep-sleep-engineer](esp32-deep-sleep-engineer/SKILL.md) | Designs deep-sleep configurations, ULP programs, and power budgets. |
| [esp32-hardware-crypto-configurer](esp32-hardware-crypto-configurer/SKILL.md) | Configures hardware AES/SHA/RSA, NVS encryption, flash encryption, and secure boot. |
| [esp32-sdd-project-validator](esp32-sdd-project-validator/SKILL.md) | Validates any example folder against the permanent locked structure; reports PASS/FAIL for every check. |

## Installing Skills

```sh
npx @agentskills/cli install RichNasz/esp32-sdd-showcase#skills
```

## Invoking a Skill

In Claude Code, reference any skill by name:

> "Activate esp32-sdd-full-project-generator skill"

## Skill Execution Order (typical new example)

```
esp32-sdd-functional-spec-creator
  └─► esp32-sdd-full-project-generator
        ├─► esp32-deep-sleep-engineer        (if sleep required)
        └─► esp32-hardware-crypto-configurer (if security required)
```

> All skills follow the 100% AI-first rule: they read specs and generate code or docs without human modification of output files.
