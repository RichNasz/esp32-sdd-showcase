<!-- ================================================
     AGENT-GENERATED — DO NOT EDIT BY HAND
     Generated from specs/ using esp32-sdd-documentation-generator skill
     Date: 2026-02-22 | Agent: Claude Code
     ================================================ -->

# Using With AI Agents

This guide covers how to use the ESP32 SDD Showcase skills with Claude Code and other AI agents. Skills are **project-local**: they live inside `.claude/skills/` in this repository and require no global installation.

## Institutional Memory — Read This First

> **Every AI agent working in this repository must read [`shared-specs/AIGenLessonsLearned.md`](../../shared-specs/AIGenLessonsLearned.md) before generating or modifying any file.**

This file is the gold standard institutional memory of the project. It contains:

- **Core rules** that must never be violated (no hand-editing generated files, always use board-specs for GPIO, etc.)
- **Key lessons** distilled from real AI generation sessions (Feb 2026)
- **Debugging workflow** for when something goes wrong (improve the spec; never patch generated code)
- **A template** for adding new lessons as the project grows

All skills enforce reading this file as their mandatory first step.

---

## Claude Code Setup

### 1. Clone the repository

```sh
git clone https://github.com/RichNasz/esp32-sdd-showcase.git
cd esp32-sdd-showcase
```

### 2. Open the project in Claude Code

Skills are automatically available once Claude Code is opened inside the repository. No install command is needed — Claude Code discovers skills from `.claude/skills/` at startup.

### 3. Invoke a skill by name

In Claude Code, activate any skill by referring to it by name:

> "Activate esp32-sdd-functional-spec-creator"

or

> "Activate esp32-sdd-full-project-generator skill"

Claude Code loads the skill's `SKILL.md` workflow and follows it step by step.

## Available Skills

| Skill | Purpose |
|---|---|
| `esp32-sdd-functional-spec-creator` | Guides creation of a new `FunctionalSpec.md` |
| `esp32-sdd-full-project-generator` | Generates a complete ESP-IDF project from specs |
| `esp32-board-spec-generator` | Produces board specs from vendor datasheets |
| `esp32-deep-sleep-engineer` | Designs sleep configurations and power budgets |
| `esp32-hardware-crypto-configurer` | Configures hardware AES, secure boot, flash encryption |
| `esp32-sdd-documentation-generator` | Regenerates all documentation from specs |
| `esp32-sdd-project-validator` | Validates any example folder against the permanent locked structure |

Full details: [skills/README.md](../../skills/README.md)

## Incremental Workflow

The typical SDD cycle when working with an AI agent:

```
1. Edit a spec file in specs/ or examples/*/FunctionalSpec.md
      │
      ▼
2. Activate the appropriate skill in Claude Code
      │
      ├─► esp32-sdd-functional-spec-creator   (new example)
      ├─► esp32-sdd-full-project-generator    (generate/update project files)
      └─► esp32-sdd-documentation-generator  (after any spec change)
      │
      ▼
3. Review agent output — never edit generated files directly
      │
      ▼
4. Build and flash
      idf.py build flash monitor
```

Always modify specs, then regenerate. If something looks wrong in a generated file, fix the spec and rerun the skill.

Every generated example must follow the permanently locked directory layout defined in `specs/example-project-structure-spec.md` — human-authored files live in `specs/`, all other files are agent-generated.

## Working with VS Code / Cursor + Official ESP-IDF Extension

Each example folder in this repository is a **complete, standalone ESP-IDF project**.

**Recommended workflow (for development):**
1. In VS Code or Cursor, choose **File → Open Folder…**
2. Navigate to and open **only the specific example folder**, for example:
   - `examples/01-blinky/`
   - `examples/03-deep-sleep-bme280-mqtt/`
   etc.

3. Once the example folder is the workspace root, the official ESP-IDF VS Code Extension will automatically detect the project and give you full support for:
   - Build / Flash / Monitor / Debug
   - Menuconfig
   - Size analysis
   - ESP-IDF commands palette

**Important notes:**
- Do **not** open the top-level `esp32-sdd-showcase` folder as your main workspace when actively working on an example (this creates a multi-root workspace and confuses the ESP-IDF extension).
- You can still open the entire repository for browsing and editing specs.
- After editing `specs/FunctionalSpec.md` or `CodingSpec.md`, regenerate the example with the full-project-generator skill, then use the ESP-IDF extension to build/flash.

## Other AI Agents (non-Claude Code)

Skills are defined as plain Markdown in `.claude/skills/<name>/SKILL.md`. Any agent that can read a Markdown workflow file can follow the same instructions:

1. Read the relevant `SKILL.md` to understand the workflow.
2. Read the spec files referenced by the workflow.
3. Generate output following the instructions in `SKILL.md`.

The skills do not depend on any Claude-specific APIs — they are prompts and structured workflows expressed in Markdown.

## Troubleshooting

**Skill not found in Claude Code**
: Confirm you opened Claude Code from the repository root. Skills must be inside `.claude/skills/` relative to the working directory.

**Generated file looks wrong**
: Do not edit the generated file. Instead, update the relevant spec in `specs/` or `examples/*/FunctionalSpec.md` and reactivate the skill.

**Unsure which skill to use**
: See the `docs/workflows/` section for the complete decision tree.

**Want to verify a generated example is structurally correct**
: Say "Activate esp32-sdd-project-validator skill" — it runs 13 checks against the permanent locked structure and reports PASS/FAIL for each.
