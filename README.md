<!-- ================================================
     AGENT-GENERATED — DO NOT EDIT BY HAND
     Generated from specs/ using esp32-sdd-documentation-generator skill
     Date: 2026-03-13 | Agent: Claude Code
     ================================================ -->

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
![ESP-IDF](https://img.shields.io/badge/ESP--IDF-v5.x-blue)
![Chip](https://img.shields.io/badge/chip-ESP32--S3-orange)
![Built with Claude Code](https://img.shields.io/badge/Built%20with-Claude%20Code-blueviolet)

# ESP32 SDD Showcase

> **100% AI-first.** Humans write specs. AI writes everything else.

ESP32 SDD Showcase is a production-grade demonstration of **Spec-Driven Development (SDD)** for
ESP32 using ESP-IDF — a discipline where humans author concise specifications and AI agents
generate all code, documentation, and build configuration from those specs. It exists to prove
that strict human/AI role separation produces better embedded firmware faster, and to give the
broader ESP32 community a reusable SDD toolkit they can fork and extend for their own projects.

## Features

- **8 progressive examples** spanning breathing LEDs through multi-year BLE beacons — every line of C generated from spec
- **7 published agent skills** covering the full SDD pipeline: spec creation, code generation, deep sleep, hardware crypto, board specs, documentation, and validation
- **AI-generated board specs** from vendor datasheets for Adafruit, Seeed, Espressif, LilyGo, and more
- **Strict human/AI separation** — generated files carry agent headers; only `specs/` is human-authored
- **Full OSS governance** — Contributing guide, Code of Conduct, Security policy, and GitHub issue templates

## Table of Contents

- [Quick Start](#quick-start)
- [Prerequisites](#prerequisites)
- [Examples](#examples)
- [Skills](#skills)
- [Full Documentation](#full-documentation)
- [Contributing](#contributing)
- [License](#license)

## Quick Start

```sh
git clone https://github.com/RichNasz/esp32-sdd-showcase
cd esp32-sdd-showcase
# Skills are project-local — no install needed.
# Open in Claude Code, then say:
# "Activate esp32-sdd-full-project-generator skill"
```

To build an existing example:

```sh
cd examples/blinky
idf.py set-target esp32s3
idf.py build flash monitor
```

## Prerequisites

- [ESP-IDF v5.x](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/) — installed and sourced in your shell
- [Claude Code](https://claude.ai/code) — for activating skills and driving the SDD workflow
- One of the supported boards: [Seeed XIAO ESP32S3](https://www.seeedstudio.com/XIAO-ESP32S3-p-5627.html), [Seeed XIAO ESP32-C5](https://www.seeedstudio.com/Seeed-Studio-XIAO-ESP32C5-p-5890.html), [Seeed XIAO ESP32-C6](https://www.seeedstudio.com/Seeed-Studio-XIAO-ESP32C6-p-5884.html), or [Adafruit HUZZAH32](https://www.adafruit.com/product/3405) — board selection is a Kconfig option

## Examples

All eight examples are fully generated from their spec files. They are ordered by complexity — start with `blinky` and work forward.

| # | Example | Description | Link |
|---|---|---|---|
| 1 | blinky | Multi-board breathing LED using LEDC PWM hardware timer | [examples/blinky](examples/blinky/) |
| 2 | deep-sleep-timer-node | RTC timer wakeup with NVS-backed counter persistence | [examples/deep-sleep-timer-node](examples/deep-sleep-timer-node/) |
| 3 | deep-sleep-bme280-mqtt-sensor | Duty-cycled IoT sensor node: BME280 → Wi-Fi STA → MQTT publish | [examples/deep-sleep-bme280-mqtt-sensor](examples/deep-sleep-bme280-mqtt-sensor/) |
| 4 | hardware-aes-secure-storage | AES-256 hardware crypto accelerator with NVS encrypted blob storage | [examples/hardware-aes-secure-storage](examples/hardware-aes-secure-storage/) |
| 5 | ble-beacon-deep-sleep | Duty-cycled NimBLE advertising with ext0 GPIO wakeup for multi-year battery life | [examples/ble-beacon-deep-sleep](examples/ble-beacon-deep-sleep/) |
| 6 | secure-ota-https | HTTPS OTA with TLS certificate pinning and automatic rollback on failure | [examples/secure-ota-https](examples/secure-ota-https/) |
| 7 | esp-now-low-power-mesh | Wi-Fi-free ESP-NOW peer mesh with dual-role firmware and power management | [examples/esp-now-low-power-mesh](examples/esp-now-low-power-mesh/) |
| 8 | capacitive-touch-wakeup | Capacitive touch sensor calibration and touch wakeup from deep sleep | [examples/capacitive-touch-wakeup](examples/capacitive-touch-wakeup/) |

## Skills

Seven agent skills implement the complete SDD pipeline. Invoke any skill in Claude Code by name.

| Skill | Purpose |
|---|---|
| [esp32-sdd-functional-spec-creator](skills/esp32-sdd-functional-spec-creator/SKILL.md) | Interactively guides creation of `FunctionalSpec.md` for a new example |
| [esp32-sdd-full-project-generator](skills/esp32-sdd-full-project-generator/SKILL.md) | Primary code-gen skill — produces a complete ESP-IDF project from spec files |
| [esp32-sdd-documentation-generator](skills/esp32-sdd-documentation-generator/SKILL.md) | Generates all Markdown documentation from `specs/`; run after any spec change |
| [esp32-board-spec-generator](skills/esp32-board-spec-generator/SKILL.md) | Creates structured board-spec Markdown from vendor datasheets |
| [esp32-deep-sleep-engineer](skills/esp32-deep-sleep-engineer/SKILL.md) | Designs deep-sleep configurations, ULP programs, and power budgets |
| [esp32-hardware-crypto-configurer](skills/esp32-hardware-crypto-configurer/SKILL.md) | Configures hardware AES/SHA/RSA, NVS encryption, flash encryption, and secure boot |
| [esp32-sdd-project-validator](skills/esp32-sdd-project-validator/SKILL.md) | Validates any example folder against the permanent locked structure; reports PASS/FAIL |

## Full Documentation

| Section | Link |
|---|---|
| Documentation hub | [docs/README.md](docs/README.md) |
| SDD workflow guide | [docs/workflows/README.md](docs/workflows/README.md) |
| Using with AI agents | [docs/guides/using-with-ai-agents.md](docs/guides/using-with-ai-agents.md) |
| Creating new board specs | [docs/guides/creating-new-board-specs.md](docs/guides/creating-new-board-specs.md) |
| Examples catalog | [examples/README.md](examples/README.md) |
| Board specs | [board-specs/README.md](board-specs/README.md) |
| Skills reference | [skills/README.md](skills/README.md) |
| Specs (human-authored) | [specs/README.md](specs/README.md) |

## Contributing

Contributions are welcome — but this project follows strict SDD rules. Only files inside `specs/`
and `examples/*/specs/` are human-authored; everything else is agent-generated and must not be
edited by hand. If something looks wrong in a generated file, fix the driving spec and regenerate.
See [CONTRIBUTING.md](CONTRIBUTING.md) for the full workflow, commit convention, and PR checklist.

## License

MIT — see [LICENSE](LICENSE).

---

*Generated by [esp32-sdd-documentation-generator](skills/esp32-sdd-documentation-generator/SKILL.md)*
