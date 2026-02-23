---
name: esp32-sdd-project-validator
description: Validates any example folder against the permanent locked structure in specs/example-project-structure-spec.md. Checks structure, spec file content, multi-board guards, agent-generated headers, and AIGenLessonsLearned.md compliance. Reports PASS/FAIL for every check.
version: "1.0.0"
author: Richard Naszcyniec
keywords: sdd, esp32, validator, structure, compliance, audit
---

<!-- ================================================
     AGENT-GENERATED — DO NOT EDIT BY HAND
     Generated from specs/ using esp32-sdd-documentation-generator skill
     Date: 2026-02-22 | Agent: Claude Code
     ================================================ -->

# ESP32 SDD Project Validator

## When to use
- Before committing or submitting a PR — verify a generated example meets all SDD rules.
- After running `esp32-sdd-full-project-generator` — confirm the output is correct.
- When debugging a build failure — rule out a structural or spec compliance issue first.
- Any time you want to audit an example folder on demand.

## Workflow (follow exactly)
MANDATORY: First read and follow `shared-specs/AIGenLessonsLearned.md` before doing anything else.

1. Read `specs/example-project-structure-spec.md` — load the permanent locked structure.
2. Read `shared-specs/AIGenLessonsLearned.md` — load the core rules.
3. List all files inside the target example folder (e.g. `examples/blinky/`).
4. Run every check below in order. For each check record: ✅ PASS, ❌ FAIL, or ⚠️ WARNING.

---

## Check Catalogue

### CHECK-01 — Required files present
Verify that every required path exists (relative to the example root):

| Path | Required? |
|---|---|
| `specs/FunctionalSpec.md` | Required |
| `specs/CodingSpec.md` | Required |
| `main/main.c` | Required |
| `main/CMakeLists.txt` | Required |
| `CMakeLists.txt` | Required |
| `sdkconfig.defaults` | Required |
| `README.md` | Required |
| `.gitignore` | Required |
| `Kconfig.projbuild` | Allowed (present when Kconfig options needed) |
| `idf_component.yml` | Allowed (present when external deps needed) |

**FAIL** if any Required file is absent.

### CHECK-02 — specs/ directory contains only human-authored files
`specs/` must contain `FunctionalSpec.md` and `CodingSpec.md`. `TestSpec.md` is also allowed (and encouraged). No other files, no subdirectories.

**FAIL** if any file other than `FunctionalSpec.md`, `CodingSpec.md`, or `TestSpec.md` is found inside `specs/`.

### CHECK-03 — No generated files have been placed inside specs/
`specs/FunctionalSpec.md` and `specs/CodingSpec.md` must NOT start with the agent-generated comment header (`<!-- AGENT-GENERATED` or `/* AGENT-GENERATED`).

**FAIL** if either spec file starts with an agent-generated header.

### CHECK-04 — Agent-generated header present in generated Markdown files
Every generated `.md` file at the example root (`README.md`) must begin with:
```
<!-- ================================================
     AGENT-GENERATED — DO NOT EDIT BY HAND
```
**FAIL** if `README.md` is missing this header.

### CHECK-05 — Agent-generated comment present in main.c
`main/main.c` must begin with `/* AGENT-GENERATED` on line 1.

**FAIL** if the comment is absent.

### CHECK-06 — AIGenLessonsLearned.md referenced in CodingSpec.md
`specs/CodingSpec.md` line 1 must contain a reference to `shared-specs/AIGenLessonsLearned.md`.

**FAIL** if the reference is absent.

### CHECK-07 — .gitignore correctness
`.gitignore` must exclude all of: `build/`, `sdkconfig`, `sdkconfig.old`, `*.bin`, `*.elf`, `*.map`.

**FAIL** if any of these entries is missing.

### CHECK-08 — sdkconfig.defaults sets at least one CONFIG_ key
`sdkconfig.defaults` must be non-empty and contain at least one `CONFIG_` assignment.

**FAIL** if the file is empty or contains no `CONFIG_` lines.

### CHECK-09 — Root CMakeLists.txt is minimal and correct
`CMakeLists.txt` must contain:
- `cmake_minimum_required`
- `include($ENV{IDF_PATH}/tools/cmake/project.cmake)`
- `project(<name>)`

**FAIL** if any of these three lines is absent.

### CHECK-10 — main/CMakeLists.txt uses idf_component_register
`main/CMakeLists.txt` must call `idf_component_register`.

**FAIL** if the call is absent.

### CHECK-11 — Multi-board guard (if Kconfig.projbuild exists)
If `Kconfig.projbuild` is present:
- It must contain a `choice` block.
- `main/main.c` must contain at least one `#if CONFIG_BOARD_` guard and a matching `#error` fallback.
- The `#error` message must instruct the user to set `CONFIG_BOARD_*` in `sdkconfig.defaults`.

**FAIL** if `Kconfig.projbuild` exists but `main.c` has no `#if CONFIG_BOARD_` guard.
**FAIL** if no `#error` fallback exists.

### CHECK-12 — Board-spec references in FunctionalSpec.md
If any board is mentioned in `FunctionalSpec.md`, the corresponding `board-specs/<vendor>/<board>.md` file must exist.

**FAIL** if a referenced board-spec file is not found on disk.

### CHECK-13 — No extra top-level files
Only these names are permitted at the example root:
`specs/`, `main/`, `CMakeLists.txt`, `sdkconfig.defaults`, `README.md`, `.gitignore`, `Kconfig.projbuild`, `idf_component.yml`, `components/`, `build/`

**WARNING** (not FAIL) for any other file at the root — report the unexpected filename so the user can decide.

---

## Output Format

After all checks, output a summary table:

```
Validation Report — examples/<name>/
─────────────────────────────────────────────────────
CHECK-01  Required files present           ✅ PASS
CHECK-02  specs/ contains only human files ✅ PASS
CHECK-03  No generated files in specs/     ✅ PASS
CHECK-04  README.md agent header           ✅ PASS
CHECK-05  main.c agent comment             ✅ PASS
CHECK-06  AIGenLessonsLearned ref          ✅ PASS
CHECK-07  .gitignore completeness          ✅ PASS
CHECK-08  sdkconfig.defaults non-empty     ✅ PASS
CHECK-09  Root CMakeLists.txt              ✅ PASS
CHECK-10  main/CMakeLists.txt              ✅ PASS
CHECK-11  Multi-board guard                ✅ PASS
CHECK-12  Board-spec files exist           ✅ PASS
CHECK-13  No extra top-level files         ✅ PASS
─────────────────────────────────────────────────────
Result: 13/13 PASS  |  0 FAIL  |  0 WARNING
```

Then output one of:
- `"✅ <name> passed all SDD structure checks."` — if zero FAILs.
- `"❌ <name> failed <N> check(s). Fix the spec(s) and regenerate."` — if any FAILs.

Do NOT suggest editing generated files. All fixes go through the spec → regenerate cycle.
