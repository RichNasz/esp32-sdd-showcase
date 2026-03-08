<!-- ================================================
     AGENT-GENERATED — DO NOT EDIT BY HAND
     Generated from specs/ using esp32-sdd-documentation-generator skill
     Date: 2026-03-08 | Agent: Claude Code
     ================================================ -->

# Creating New Board Specs

This guide explains how to add a new ESP32 board to the `board-specs/` catalog using the **esp32-board-spec-generator** skill. The Seeed XIAO ESP32S3 is used as a live worked example throughout.

---

## Overview

A board spec is an authoritative, structured Markdown file that describes the hardware layout of one specific development board or module. It is the single source of truth for GPIO numbers, power limits, strapping pin constraints, and peripheral assignments. All example projects reference board specs instead of hard-coding pin numbers.

Location: `board-specs/<vendor-slug>/<board-slug>.md`

---

## When to Use esp32-board-spec-generator

| Situation | Action |
|---|---|
| Adding a new board to a `FunctionalSpec.md` that has no existing board spec | Generate the spec first, then write the FunctionalSpec |
| Existing board spec is missing sections (power, strapping, ADC warnings) | Regenerate — never hand-edit |
| Vendor released a hardware revision with changed pin assignments | Regenerate with updated source URL |
| A board spec was created as a placeholder with unverified data | Regenerate with verified authoritative source |

---

## What to Provide to the Agent

When invoking the skill, include all of the following in your prompt:

### 1. Authoritative source URL
Prefer the **vendor's official wiki** over GitHub READMEs or third-party guides:
- Seeed Studio: `wiki.seeedstudio.com`
- Adafruit: `learn.adafruit.com`
- Espressif: `docs.espressif.com` + devkit product pages

### 2. Key facts to verify before running
Cross-check these against the vendor schematic or wiki before handing them to the agent — they are the highest-risk items for hallucination:

| Fact | How to verify |
|---|---|
| Onboard LED GPIO number and polarity | Vendor wiki "Pinout" or schematic net list |
| Strapping pin numbers | ESP32 Technical Reference Manual + board schematic |
| PSRAM type (none / SPI / OPI) and size | Module datasheet or `idf.py menuconfig` → Component config → ESP PSRAM |
| Deep sleep current | Vendor datasheet power table |
| ADC1 vs ADC2 pin assignments | ESP32 family TRM, "ADC" chapter |

### 3. Board name slug
Use lowercase-kebab: `xiao-esp32s3`, `huzzah32`, `esp32-devkitc-v4`.

### Example invocation

```
Activate esp32-board-spec-generator.

Source: https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/
Board: Seeed XIAO ESP32S3

Pre-verified facts:
- Onboard LED: GPIO21, active LOW
- Strapping pins: GPIO0 (boot), GPIO45 (VDD_SPI), GPIO46 (ROM log)
- PSRAM: 8 MB OPI
- Deep sleep: ~14 µA
- ADC2 disabled when Wi-Fi active; all D-pads use ADC1
```

---

## Board Spec Template (Required Sections)

Every board spec must contain these sections in this order:

| Section | Purpose |
|---|---|
| Agent-generated header | Marks file as generated; includes source URL and date |
| **Module / SoC** | Chip variant, flash, PSRAM, antenna type, form factor |
| **Power Management** | 3V3 regulator limit, USB input, battery charger, deep sleep µA |
| **Onboard Peripherals** | GPIO-mapped peripherals (LEDs, buttons, sensors) with polarity |
| **GPIO & Pin Rules / Warnings** | Strapping pins, ADC restrictions, reserved internal pins, USB-CDC requirements |
| **Complete Pin Mapping Table** | All user-accessible pads with GPIO, ADC channel, and default peripheral |
| **Board Advantages for SDD Projects** | Why this board suits which types of SDD examples |
| **idf.py Target** | The exact `idf.py set-target <chip>` command |

---

## Best Practices

**Verify the LED GPIO with a blink sketch first.**
Board wikis sometimes describe an older PCB revision. Flash a simple `gpio_set_level(21, 0)` sketch and confirm the LED lights before accepting the GPIO number as authoritative.

**Prefer the vendor wiki over community GitHub repos.**
GitHub READMEs are often outdated. The vendor's official wiki is versioned and maintained.

**Note ADC limitations explicitly.**
ESP32 ADC2 is disabled when Wi-Fi is active. State which user pads are ADC1 vs ADC2 — this prevents hard-to-debug analog read failures in Wi-Fi-enabled projects.

**List strapping pins even if they seem obvious.**
Strapping pins are the #1 cause of "my board won't boot" issues. Every board spec must list them regardless of how common the board is.

**For active-LOW LEDs, note the LEDC duty inversion.**
If the onboard LED is active LOW (common on XIAO and many compact boards), note it explicitly so the full-project-generator inverts LEDC duty correctly: duty 0 = full brightness, duty max = off.

**Never omit a section because the board "probably" doesn't have it.**
If no PSRAM: write `None`. If no LiPo charger: write `None`. Absence of information is information.

---

## Live Example: Seeed XIAO ESP32S3

This section documents exactly how `board-specs/seeed/xiao-esp32s3.md` was created, including what the initial placeholder got wrong and why.

### Step 1 — Identify the authoritative source

Source used: `https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/`

The Seeed wiki provides:
- Full pinout diagram with D-pin labels and GPIO numbers
- Schematic download link (confirms LED polarity)
- Power consumption table (active, modem-sleep, deep sleep)
- Hardware overview with PSRAM and flash details

### Step 2 — Extract and verify key facts before running the skill

| Fact | Raw wiki value | Verification method |
|---|---|---|
| LED GPIO | GPIO21 | Confirmed in wiki pinout diagram and schematic net "USER_LED" → GPIO21 |
| LED polarity | Active LOW | Schematic: LED anode → 3V3, cathode → GPIO21 via resistor |
| D0→GPIO mapping | D0=GPIO1 | Wiki pinout table cross-checked against ESP32-S3R8 datasheet |
| D6/D7 GPIOs | D6=GPIO43, D7=GPIO44 | Wiki pinout (UART0 TX/RX) |
| PSRAM | 8 MB OPI | "ESP32-S3R8" part number: R = PSRAM, 8 = 8 MB |
| Deep sleep | ~14 µA | Wiki power table |
| ADC2 + Wi-Fi | ADC2 disabled | ESP32-S3 TRM, section "ADC"; confirmed all D-pads use ADC1 |

### Step 3 — What the initial placeholder got wrong

The first version of `board-specs/seeed/xiao-esp32s3.md` (created as a migration placeholder) had these errors:

| Error | Incorrect value | Correct value |
|---|---|---|
| LED polarity hedge | "Active LOW on some PCB revisions" | Always active LOW — remove the hedge |
| LED GPIO note | Said "D10 pad = GPIO 10 — available for user LED in some board revisions" | GPIO10 is D10/MOSI; the LED is on GPIO21, separate from all pads |
| D6 GPIO | D6 = GPIO7 | D6 = GPIO43 (UART0 TX) |
| D7 GPIO | D7 = GPIO8 | D7 = GPIO44 (UART0 RX) |
| D8 GPIO | D8 = GPIO9 | D8 = GPIO7 (SPI SCK) |
| D9 GPIO | D9 = GPIO10 | D9 = GPIO8 (SPI MISO) |
| D10 GPIO | D10 = GPIO44 | D10 = GPIO9 (SPI MOSI) |
| Missing sections | No Power Management, no GPIO warnings, no strapping pins | All added in the authoritative regeneration |

**Root cause**: The placeholder was generated from memory without a vendor source URL. The pin mapping was essentially shifted by one from a misread of the pinout diagram.

**Lesson learned (added to `shared-specs/AIGenLessonsLearned.md`)**: Always provide an authoritative vendor URL and pre-verified key facts when invoking `esp32-board-spec-generator`. A board spec created without a source URL must be treated as unverified and regenerated before use in production firmware.

### Step 4 — The regeneration invocation

```
Activate esp32-board-spec-generator.

Source: https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/
Board: Seeed XIAO ESP32S3

Pre-verified facts (from Seeed wiki + schematic + ESP32-S3 TRM):
- Onboard User LED: GPIO21, active LOW (always — no PCB revision ambiguity)
- 3V3 regulator: up to 700 mA
- Battery charger: built-in LiPo charger, JST 1.25 mm connector
- Deep sleep current: ~14 µA
- Form factor: XIAO 14-pin (21 × 17.8 mm)
- ADC2 limitation: ADC2 disabled when Wi-Fi is active (all user pads use ADC1 — safe)
- Strapping pins: GPIO0 (boot), GPIO45 (VDD_SPI voltage), GPIO46 (ROM log)
- USB-CDC: native USB requires CONFIG_ESP_CONSOLE_USB_CDC=y for serial output
```

### Step 5 — Verify the output

After generation:

```sh
# Confirm LED GPIO
grep "GPIO 21" board-specs/seeed/xiao-esp32s3.md

# Confirm GPIO10 is NOT listed as LED (it's D10/MOSI)
grep -n "GPIO 10\|GPIO10" board-specs/seeed/xiao-esp32s3.md

# Confirm D-pin mapping
grep "D6\|D7\|D8\|D9\|D10" board-specs/seeed/xiao-esp32s3.md

# Confirm strapping pins present
grep "GPIO 0\|GPIO 45\|GPIO 46" board-specs/seeed/xiao-esp32s3.md

# Confirm ADC2 warning
grep -i "adc2\|wi-fi" board-specs/seeed/xiao-esp32s3.md
```

---

## Downstream Impact on Examples

When a board spec is corrected or updated:

1. Check all `FunctionalSpec.md` files that reference this board spec.
2. If GPIO assignments changed, update `CodingSpec.md` (the `# Board:` comment and any explicit GPIO references).
3. Regenerate the affected example with `esp32-sdd-full-project-generator`.
4. Run `esp32-sdd-project-validator` on the regenerated example to confirm structural compliance.

> **blinky reference**: The XIAO ESP32S3 LED is GPIO21 (active LOW). The `examples/blinky` example has been regenerated with the correct pin and now also supports XIAO ESP32-C5 (GPIO 27, active LOW) and XIAO ESP32-C6 (GPIO 15, active LOW).
