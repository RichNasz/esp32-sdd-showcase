<!-- ================================================
     AGENT-GENERATED — DO NOT EDIT BY HAND
     Generated from specs/ using esp32-sdd-documentation-generator skill
     Date: 2026-02-22 | Agent: Claude Code
     ================================================ -->

# Guides

Step-by-step guides for getting started with the ESP32 SDD Showcase, from environment setup through creating your own SDD examples.

## Available Guides

| Guide | Description |
|---|---|
| [01 — Toolchain Setup](01-toolchain-setup.md) | Install ESP-IDF, the Xtensa toolchain, and verify your environment |
| [02 — Install Skills](02-install-skills.md) | Install the agent skill suite with `@agentskills/cli` |
| [03 — Run Your First Example](03-first-example.md) | Build and flash `01-blinky-huzzah32` from scratch |
| [04 — Create a New Example](04-new-example.md) | Use `esp32-sdd-functional-spec-creator` to spec and generate a new project |
| [05 — Add a Board Spec](05-add-board-spec.md) | Use `esp32-board-spec-generator` to add your board to the catalog |
| [06 — Deep Sleep Design](06-deep-sleep-design.md) | Use `esp32-deep-sleep-engineer` to hit your power budget |
| [07 — Secure Your Project](07-secure-your-project.md) | Use `esp32-hardware-crypto-configurer` for hardware AES, OTA signing, and secure boot |
| [Using With AI Agents](using-with-ai-agents.md) | Project-local skill setup for Claude Code and other AI agents |

## Recommended Reading Order

If you are new to the project, follow the guides in order:

```
01 Toolchain Setup
  └─► 02 Install Skills
        └─► 03 Run Your First Example
              └─► 04 Create a New Example
```

Then dive into topic guides (05–07) as needed for your project.

## Prerequisites

- macOS, Linux, or Windows WSL2
- Python 3.9+
- Git
- Claude Code (skills are project-local — no global install needed)
- A USB-to-serial adapter or a devkit with built-in USB (CP2102, CH340, or native USB CDC)
