---
name: esp32-board-spec-generator
description: Generates structured board-spec Markdown files from vendor documentation. Produces pinout tables, peripheral maps, power budgets, flash/PSRAM inventory, and antenna notes for any ESP32-family module or devkit.
version: "1.0.0"
author: Richard Naszcyniec
keywords: sdd, esp32, board-spec, hardware, pinout, vendor, datasheet
---

<!-- ================================================
     AGENT-GENERATED — DO NOT EDIT BY HAND
     Generated from specs/ using esp32-sdd-documentation-generator skill
     Date: 2026-02-22 | Agent: Claude Code
     ================================================ -->

# ESP32 Board Spec Generator

## When to use
When adding a new ESP32 board or module to the project, or when vendor documentation has changed and the board spec needs to be refreshed.

## Inputs
- Vendor product page URL or datasheet PDF (provide in the prompt)
- Board/module name and variant (e.g. "Adafruit HUZZAH32 – ESP32 Feather v2")

## Workflow (follow exactly)
MANDATORY: First read and follow `shared-specs/AIGenLessonsLearned.md` before doing anything else.

1. Parse the provided vendor documentation (datasheet, product page, or schematic).
2. Extract and normalise: full GPIO pinout, peripheral assignments (I2C, SPI, UART, PWM, ADC, DAC, touch), power supply rails and limits, flash/PSRAM sizes and interface speeds, antenna type, and any reserved or strapping pins.
3. Cross-reference against `shared-specs/esp-idf-version.md` for any platform-level constraints.
4. Generate `board-specs/<vendor>/<board-name>.md` using the standard section layout from `templates/BoardSpec.template.md`.
5. Add the standard agent-generated header to the output file.
6. Output: file path written and a one-line summary of key specs (chip variant, flash, PSRAM, antenna).
