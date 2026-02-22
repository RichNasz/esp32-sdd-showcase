# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What This Repo Is

A **Spec-Driven Development (SDD)** showcase for ESP32 / ESP-IDF. The strict rule: humans write specs, AI generates everything else. Never edit agent-generated files directly — modify the relevant spec and regenerate.

## The Two File Categories

| Category | Locations | Rule |
|---|---|---|
| **Human-authored** | `specs/`, `shared-specs/`, `examples/*/FunctionalSpec.md`, `examples/*/CodingSpec.md` | Edit freely |
| **Agent-generated** | Everything else (all `README.md`, all C source, `CMakeLists.txt`, `sdkconfig.defaults`, `skills/*/SKILL.md`, `board-specs/`) | Never edit by hand — regenerate |

## Skills (invoke by name in Claude Code)

| Skill | When to use |
|---|---|
| `esp32-sdd-documentation-generator` | After any change to `specs/` or `shared-specs/` — regenerates all Markdown |
| `esp32-sdd-functional-spec-creator` | Starting a new example — creates `examples/<name>/FunctionalSpec.md` |
| `esp32-sdd-full-project-generator` | After FunctionalSpec + CodingSpec exist — generates the complete ESP-IDF project |
| `esp32-board-spec-generator` | Adding a new board — creates `board-specs/<vendor>/<board>.md` from a datasheet URL |
| `esp32-deep-sleep-engineer` | Auto-invoked by full-project-generator when sleep is required; can also be run standalone |
| `esp32-hardware-crypto-configurer` | Auto-invoked by full-project-generator when security is required; can also be run standalone |

Skills are defined in `skills/*/SKILL.md`. Each SKILL.md contains its exact workflow — read it before invoking.

## Building Examples (once ESP-IDF project files exist)

```sh
cd examples/<name>
idf.py set-target esp32          # or esp32s2, esp32s3, esp32c3
idf.py build
idf.py flash monitor             # flash and open serial monitor
idf.py menuconfig                # adjust CONFIG_* options interactively
```

The `.gitignore` excludes `build/`, `sdkconfig`, `sdkconfig.old`, `*.bin`, `*.elf`, `*.map` — never commit these.

## Architecture

```
specs/              ← source of truth for the whole project
shared-specs/       ← platform-wide constraints (ESP-IDF version, coding conventions, power budget templates)
examples/<name>/    ← one folder per example; FunctionalSpec.md drives everything generated inside it
board-specs/<vendor>/<board>.md  ← generated from vendor datasheets; referenced by FunctionalSpec.md
skills/<name>/SKILL.md  ← skill workflow definitions; read these to understand what each skill does
docs/               ← full documentation hub; docs/workflows/README.md explains all SDD workflows
templates/          ← structural templates used by skills during spec instantiation
```

## Workflow Summary

1. **New example**: edit `specs/showcase-master-spec.md` → activate `esp32-sdd-functional-spec-creator` → add `CodingSpec.md` → activate `esp32-sdd-full-project-generator`
2. **Change specs**: edit file in `specs/` or `shared-specs/` → activate `esp32-sdd-documentation-generator`
3. **New board**: provide datasheet URL → activate `esp32-board-spec-generator`

## Documentation Regeneration

After any spec change, the canonical trigger phrase is:

> "Activate esp32-sdd-documentation-generator and rebuild all documentation."

Every generated Markdown file must begin with the standard agent-generated header (see any existing `README.md` for the exact format).
