<!-- ================================================
     AGENT-GENERATED — DO NOT EDIT BY HAND
     Generated from specs/ using esp32-sdd-documentation-generator skill
     Date: 2026-02-22 | Agent: Claude Code
     ================================================ -->

# templates/

Canonical structural templates for creating new specs and project files. Agent skills use these as starting points during generation; they also serve as a reference for humans authoring new `FunctionalSpec.md` or `CodingSpec.md` files.

## Available Templates

| Template | Used By | Purpose |
|---|---|---|
| `FunctionalSpec.template.md` | esp32-sdd-functional-spec-creator | Structure and required sections for a new example functional specification |
| `CodingSpec.template.md` | esp32-sdd-full-project-generator | Structure for implementation-level coding decisions and component breakdown |
| `BoardSpec.template.md` | esp32-board-spec-generator | Standard section layout for a new board specification |
| `SharedSpec.template.md` | — | Structure for a new shared platform specification |

## Usage

Templates are read automatically by their associated skill — you do not need to copy or instantiate them manually. The skills handle template expansion and output to the correct destination.

If you want to see what sections a `FunctionalSpec.md` needs before running the creator skill, `FunctionalSpec.template.md` is the reference.

## Editing Templates

Only modify template files when the project's structural conventions change (e.g. adding a required section to all functional specs). After any template change:

> "Activate esp32-sdd-documentation-generator and rebuild all documentation."
