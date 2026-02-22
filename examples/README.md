<!-- ================================================
     AGENT-GENERATED — DO NOT EDIT BY HAND
     Generated from specs/ using esp32-sdd-documentation-generator skill
     Date: 2026-02-22 | Agent: Claude Code
     ================================================ -->

# examples/

Eight progressive ESP32 examples, each fully generated from its `FunctionalSpec.md` and `CodingSpec.md` by the **esp32-sdd-full-project-generator** skill. They are ordered by complexity — start with `01-blinky-huzzah32` and work forward.

## Catalog

| # | Example | Core Concept | Key ESP32 Hardware Feature |
|---|---|---|---|
| 01 | [blinky-huzzah32](01-blinky-huzzah32/) | Baseline project structure | LEDC PWM breathing LED |
| 02 | [deep-sleep-timer-node](02-deep-sleep-timer-node/) | RTC wakeup + NVS persistence | Deep sleep timer wakeup |
| 03 | [deep-sleep-bme280-mqtt](03-deep-sleep-bme280-mqtt/) | Real IoT sensor node | BME280 + Wi-Fi + MQTT |
| 04 | [hardware-aes-secure-storage](04-hardware-aes-secure-storage/) | Crypto accelerator | AES-256 hardware engine |
| 05 | [ble-beacon-deep-sleep](05-ble-beacon-deep-sleep/) | Multi-year battery life | BLE advertising + deep sleep |
| 06 | [secure-ota-https](06-secure-ota-https/) | Signed production updates | HTTPS OTA + signature verify |
| 07 | [esp-now-low-power-mesh](07-esp-now-low-power-mesh/) | Wi-Fi-free mesh networking | ESP-NOW + power management |
| 08 | [capacitive-touch-wakeup](08-capacitive-touch-wakeup/) | Touch-driven interfaces | Touch sensor wakeup |

## Example Structure

Every example folder follows this exact, permanently locked layout (see `specs/example-project-structure-spec.md`):

```
examples/<kebab-name>/
├── specs/                          ← Human-only (FunctionalSpec.md + CodingSpec.md)
├── main/                           ← Official ESP-IDF main component (100% agent-generated)
│   ├── main.c
│   └── CMakeLists.txt
├── CMakeLists.txt                  ← Root CMakeLists.txt (generated)
├── sdkconfig.defaults              ← ESP-IDF config (generated)
├── idf_component.yml               ← Dependencies (generated when needed)
├── README.md                       ← Generated
├── .gitignore                      ← Generated
└── build/                          ← gitignored
```

## Building an Example

### Command Line

```sh
cd examples/01-blinky-huzzah32
idf.py set-target esp32
idf.py build flash monitor
```

Replace `esp32` with `esp32s2`, `esp32s3`, or `esp32c3` as appropriate for your board.

### VS Code / Cursor + ESP-IDF Extension

Each example folder is a **complete, standalone ESP-IDF project**. For the best experience:

1. **File → Open Folder…** and select the specific example folder (e.g. `examples/01-blinky-huzzah32/`)
2. The ESP-IDF Extension auto-detects the project and enables Build / Flash / Monitor / Debug from the commands palette.

> Do **not** open the repository root as your active workspace when developing an example — this creates a multi-root workspace that confuses the ESP-IDF Extension. Open the entire repository only for browsing specs.

See [docs/guides/using-with-ai-agents.md](../docs/guides/using-with-ai-agents.md) for full IDE setup details.

## Generating a New Example

1. Create the example folder and its `specs/` subdirectory.
2. Create spec files (say: **"Activate esp32-sdd-functional-spec-creator skill"**) — output goes in `specs/FunctionalSpec.md`.
3. Add `specs/CodingSpec.md`.
4. Say: **"Activate esp32-sdd-full-project-generator skill"**

> `specs/FunctionalSpec.md` and `specs/CodingSpec.md` are the only files humans write. Everything else is agent-generated and must not be edited by hand.
