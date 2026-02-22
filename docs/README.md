<!-- ================================================
     AGENT-GENERATED — DO NOT EDIT BY HAND
     Generated from specs/ using esp32-sdd-documentation-generator skill
     Date: 2026-02-22 | Agent: Claude Code
     ================================================ -->

# ESP32 SDD Showcase — Full Documentation

> **100% AI-first.** Every line of code and every piece of documentation in this repository is generated exclusively by AI agents from human-written specifications in `specs/`.

## What Is This Project?

The ESP32 SDD Showcase is a reference implementation of **Spec-Driven Development (SDD)** targeting the ESP32 microcontroller family with ESP-IDF 5.x. It proves that a complete, production-quality embedded firmware project — with deep sleep, hardware crypto, BLE, OTA, and mesh networking — can be built without a human ever writing a line of C or Markdown outside of specification files.

## Documentation Sections

| Section | Description |
|---|---|
| [Guides](guides/README.md) | Step-by-step tutorials: setting up the toolchain, running your first example, adding a new board |
| [Reference](reference/README.md) | API quick-reference, pin mapping tables, sdkconfig option index |
| [Workflows](workflows/README.md) | The complete SDD workflow: from spec to flashed firmware |

## Examples Catalog

| # | Example | Concept |
|---|---|---|
| 01 | [blinky-huzzah32](../examples/01-blinky-huzzah32/) | Baseline: SDD structure + LEDC PWM |
| 02 | [deep-sleep-timer-node](../examples/02-deep-sleep-timer-node/) | RTC wakeup + NVS persistence |
| 03 | [deep-sleep-bme280-mqtt](../examples/03-deep-sleep-bme280-mqtt/) | IoT sensor node: BME280 + Wi-Fi + MQTT |
| 04 | [hardware-aes-secure-storage](../examples/04-hardware-aes-secure-storage/) | AES-256 hardware accelerator |
| 05 | [ble-beacon-deep-sleep](../examples/05-ble-beacon-deep-sleep/) | BLE beacon + multi-year battery life |
| 06 | [secure-ota-https](../examples/06-secure-ota-https/) | HTTPS OTA + RSA signature verification |
| 07 | [esp-now-low-power-mesh](../examples/07-esp-now-low-power-mesh/) | ESP-NOW Wi-Fi-free mesh |
| 08 | [capacitive-touch-wakeup](../examples/08-capacitive-touch-wakeup/) | Capacitive touch wakeup |

## Agent Skills

Six skills implement the full SDD pipeline. Skills are **project-local** — they live in `.claude/skills/` inside this repository and are available in Claude Code automatically when you open the project. No install command is needed.

See [docs/guides/using-with-ai-agents.md](guides/using-with-ai-agents.md) for Claude Code setup and workflow details.

| Skill | Purpose |
|---|---|
| `esp32-sdd-functional-spec-creator` | Guides creation of a new `FunctionalSpec.md` |
| `esp32-sdd-full-project-generator` | Generates a complete ESP-IDF project from specs |
| `esp32-board-spec-generator` | Produces board specs from vendor datasheets |
| `esp32-deep-sleep-engineer` | Designs sleep configurations and power budgets |
| `esp32-hardware-crypto-configurer` | Configures hardware AES, secure boot, flash encryption |
| `esp32-sdd-documentation-generator` | Regenerates all documentation from specs (this file!) |

See [skills/README.md](../skills/README.md) for full details.

## Repository Layout

```
esp32-sdd-showcase/
├── specs/              ← HUMAN-AUTHORED: source of truth for everything
├── shared-specs/       ← HUMAN-AUTHORED: cross-example platform constraints
├── examples/           ← AGENT-GENERATED projects (FunctionalSpec.md is human)
├── board-specs/        ← AGENT-GENERATED board hardware specs
├── skills/             ← AGENT-GENERATED skill definitions
├── templates/          ← AGENT-GENERATED spec templates
├── docs/               ← AGENT-GENERATED documentation (you are here)
└── .github/workflows/  ← AGENT-GENERATED CI/CD
```

## The SDD Workflow in Three Steps

1. **Write a spec** — edit or create a file in `specs/` or `examples/*/specs/FunctionalSpec.md`
2. **Activate a skill** — say "Activate esp32-sdd-full-project-generator skill" in Claude Code
3. **Build and flash** — open the example folder in VS Code / Cursor and use the ESP-IDF Extension, or run `idf.py build flash monitor` from the example directory

Full workflow details: [docs/workflows/README.md](workflows/README.md)

## IDE Integration

Each example is a standalone ESP-IDF project. For the best development experience open **only the example folder** (e.g. `examples/01-blinky-huzzah32/`) in VS Code or Cursor — not the repository root. The official ESP-IDF Extension will then auto-detect the project and enable one-click build, flash, monitor, and debug.

See [guides/using-with-ai-agents.md](guides/using-with-ai-agents.md) for full setup details.

## Contributing

Only `specs/` and `examples/*/FunctionalSpec.md` / `examples/*/CodingSpec.md` are open to human edits. Everything else is agent-generated. After any spec change:

> "Activate esp32-sdd-documentation-generator and rebuild all documentation."

See [specs/contributing-spec.md](../specs/contributing-spec.md) for the full contribution rules.
