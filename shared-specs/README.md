<!-- ================================================
     AGENT-GENERATED — DO NOT EDIT BY HAND
     Generated from specs/ using esp32-sdd-documentation-generator skill
     Date: 2026-02-22 (updated) | Agent: Claude Code
     ================================================ -->

# shared-specs/

Specifications that apply across multiple examples or the entire project. These files capture platform-level constraints, coding standards, and reusable hardware patterns. Individual example `FunctionalSpec.md` files reference these by name; the **esp32-sdd-full-project-generator** skill reads them automatically during project generation.

## Essential Reading

> **Every AI agent working in this repository must read `AIGenLessonsLearned.md` before generating or modifying any file.** It is the gold standard institutional memory and quality guardrail for the entire project.

| File | Purpose |
|---|---|
| **[`AIGenLessonsLearned.md`](AIGenLessonsLearned.md)** | **MANDATORY FIRST READ — Gold standard institutional memory, core rules, and lessons from real AI generation** |
| `esp-idf-version.md` | Pinned ESP-IDF version, toolchain version, and target SDK constraints used by all examples and CI |
| `coding-conventions.md` | C coding style, naming conventions, error-handling patterns, and ESP-IDF best practices |
| `power-budget-template.md` | Standard power budget calculation template for all low-power examples |
| `security-baseline.md` | Minimum security requirements and acceptable configurations for production-targeted examples |
| `cmake-conventions.md` | CMakeLists.txt structure, component layout, dependency declaration rules, and Kconfig standards |
| `partition-table-profiles.md` | Reusable partition table CSV profiles (single OTA, dual OTA, encrypted NVS, etc.) |

## Usage

Reference a shared spec in any `FunctionalSpec.md` like this:

```markdown
## Shared Specs
- shared-specs/AIGenLessonsLearned.md
- shared-specs/esp-idf-version.md
- shared-specs/security-baseline.md
```

The full-project-generator skill will pull in the referenced shared specs automatically.

## Editing

Shared spec files are **human-authored** — they live here as a primary source of truth alongside example-level specs. Edit them directly when platform-level constraints change, then regenerate all documentation.
