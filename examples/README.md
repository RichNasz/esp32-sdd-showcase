<!-- ================================================
     AGENT-GENERATED — DO NOT EDIT BY HAND
     Generated from specs/ using esp32-sdd-documentation-generator skill
     Date: 2026-03-08 | Agent: Claude Code
     ================================================ -->

# examples/

Eight progressive ESP32 examples, each fully generated from its `FunctionalSpec.md` and `CodingSpec.md` by the **esp32-sdd-full-project-generator** skill. They are ordered by complexity — start with `blinky` and work forward.

## Learning Progression

Start at example 1 and work forward — each example introduces new hardware features and builds on patterns from the previous one.

| # | Example | What You Learn | Key ESP32-S3 Feature | Complexity |
|---|---|---|---|---|
| 1 | [blinky](blinky/) | SDD pipeline, LEDC PWM, multi-board Kconfig | Hardware PWM timer | ★☆☆☆☆ |
| 2 | [deep-sleep-timer-node](deep-sleep-timer-node/) | Deep sleep, RTC timer wakeup, RTC_DATA_ATTR | Deep sleep + RTC memory | ★★☆☆☆ |
| 3 | [deep-sleep-bme280-mqtt-sensor](deep-sleep-bme280-mqtt-sensor/) | I2C sensor, Wi-Fi STA, MQTT publish, duty-cycled sleep | I2C master + Wi-Fi + MQTT | ★★★☆☆ |
| 4 | [hardware-aes-secure-storage](hardware-aes-secure-storage/) | Hardware AES-256, NVS encrypted blobs, secure API design | mbedTLS hardware AES accelerator | ★★★☆☆ |
| 5 | [ble-beacon-deep-sleep](ble-beacon-deep-sleep/) | NimBLE advertising, duty-cycled BLE, ext0 wakeup | BLE + deep sleep + ext0 | ★★★★☆ |
| 6 | [secure-ota-https](secure-ota-https/) | HTTPS OTA, TLS cert pinning, rollback protection | esp_https_ota + bootloader rollback | ★★★★☆ |
| 7 | [esp-now-low-power-mesh](esp-now-low-power-mesh/) | ESP-NOW peer comms, dual-role firmware, mesh patterns | ESP-NOW + power management | ★★★★☆ |
| 8 | [capacitive-touch-wakeup](capacitive-touch-wakeup/) | Touch sensor calibration, touch wakeup from deep sleep | Capacitive touch peripheral | ★★★★★ |

## Example Structure

Every example folder follows this exact, permanently locked layout (see `specs/example-project-structure-spec.md`):

```
examples/<kebab-name>/
├── specs/                          ← Human-only (FunctionalSpec.md + CodingSpec.md + TestSpec.md)
│   ├── FunctionalSpec.md
│   ├── CodingSpec.md
│   └── TestSpec.md                 ← Human-written testing requirements
├── main/                           ← Official ESP-IDF main component (100% agent-generated)
│   ├── main.c
│   ├── CMakeLists.txt
│   └── Kconfig.projbuild           ← Optional; required for multi-board Kconfig choices
├── CMakeLists.txt                  ← Root CMakeLists.txt (generated)
├── sdkconfig.defaults              ← ESP-IDF config (generated)
├── idf_component.yml               ← Dependencies (generated when needed)
├── README.md                       ← Generated (includes Testing section from TestSpec.md)
├── .gitignore                      ← Generated
└── build/                          ← gitignored
```

## Building an Example

### Command Line

```sh
cd examples/blinky
idf.py set-target esp32s3
idf.py build flash monitor
```

Replace `esp32s3` with `esp32`, `esp32s2`, `esp32c3`, `esp32c5`, or `esp32c6` as appropriate for your board.

### VS Code / Cursor + ESP-IDF Extension

Each example folder is a **complete, standalone ESP-IDF project**. For the best experience:

1. **File → Open Folder…** and select the specific example folder (e.g. `examples/blinky/`)
2. The ESP-IDF Extension auto-detects the project and enables Build / Flash / Monitor / Debug from the commands palette.

> Do **not** open the repository root as your active workspace when developing an example — this creates a multi-root workspace that confuses the ESP-IDF Extension. Open the entire repository only for browsing specs.

See [docs/guides/using-with-ai-agents.md](../docs/guides/using-with-ai-agents.md) for full IDE setup details.

## Generating a New Example

1. Create the example folder and its `specs/` subdirectory.
   > Need to add a new board first? See [docs/guides/creating-new-board-specs.md](../docs/guides/creating-new-board-specs.md) for the full walkthrough.
2. Create spec files (say: **"Activate esp32-sdd-functional-spec-creator skill"**) — output goes in `specs/FunctionalSpec.md`.
3. Add `specs/CodingSpec.md`.
4. Say: **"Activate esp32-sdd-full-project-generator skill"**

> `specs/FunctionalSpec.md`, `specs/CodingSpec.md`, and `specs/TestSpec.md` are the only files humans write. Everything else is agent-generated and must not be edited by hand.
