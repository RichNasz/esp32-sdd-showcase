---
name: esp32-sdd-full-project-generator
description: The primary SDD code-generation skill. Reads a FunctionalSpec.md + CodingSpec.md and emits a complete, buildable ESP-IDF 5.x project — CMakeLists, sdkconfig.defaults, main component, all supporting components, Kconfig fragments, and a generated README.
version: "1.0.0"
author: Richard Naszcyniec
keywords: sdd, esp32, project-generator, esp-idf, cmake, codegen, full-project
---

<!-- ================================================
     AGENT-GENERATED — DO NOT EDIT BY HAND
     Generated from specs/ using esp32-sdd-documentation-generator skill
     Date: 2026-02-22 | Agent: Claude Code
     ================================================ -->

# ESP32 SDD Full Project Generator

## When to use
After `FunctionalSpec.md` and `CodingSpec.md` exist for an example. This is the primary code-generation step — it produces a complete, buildable ESP-IDF project from spec inputs alone.

## Mandatory Directory Layout (PERMANENT — NEVER DEVIATE)

Every generated example **must** match this exact structure, defined in `specs/example-project-structure-spec.md`:

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

Do not create files outside this layout. Do not place `FunctionalSpec.md` or `CodingSpec.md` at the example root — they belong in `specs/`. Do not add extra top-level files unless they are in this list.

## Workflow (follow exactly)
MANDATORY: First read and follow `shared-specs/AIGenLessonsLearned.md` before doing anything else.

1. Read `examples/<name>/specs/FunctionalSpec.md` and `examples/<name>/specs/CodingSpec.md`.
2. Read the relevant `board-specs/<vendor>/<board>.md` file identified in the FunctionalSpec.
3. Read any `shared-specs/` files referenced in either spec.
4. Generate the full project tree using the mandatory layout above:
   - `CMakeLists.txt` (top-level)
   - `sdkconfig.defaults` (all required CONFIG_ keys)
   - `main/main.c` and `main/CMakeLists.txt`
   - All components under `components/<name>/` with their own `CMakeLists.txt`
   - `Kconfig.projbuild` if custom menuconfig options are needed
   - `idf_component.yml` if external component dependencies are required
   - `.gitignore` (must exclude `build/`, `sdkconfig`, `sdkconfig.old`, `*.bin`, `*.elf`, `*.map`)
5. Invoke `esp32-deep-sleep-engineer` sub-workflow if deep sleep is specified in the FunctionalSpec.
6. Invoke `esp32-hardware-crypto-configurer` sub-workflow if security requirements are present.
7. Verify internal consistency: pin assignments match the board spec, component dependencies match CMakeLists, all FunctionalSpec features are accounted for.
8. Generate `examples/<name>/README.md` with agent-generated header and all of the following sections in order:
   - **Overview** — one-paragraph summary of what the example demonstrates
   - **Prerequisites** — ESP-IDF version, target chip, required hardware
   - **Build & Flash** — `idf.py set-target`, `idf.py build flash monitor` commands
   - **Opening in VS Code / Cursor** — the following exact subsection must appear in every generated README:

     ```
     ### Opening in VS Code / Cursor

     This folder is a complete, standalone ESP-IDF project. Open **only this folder**
     (not the repository root) in VS Code or Cursor so the official ESP-IDF Extension
     can detect and configure the project automatically.

     **File → Open Folder… → select `examples/<name>/`**

     This gives you one-click Build / Flash / Monitor / Debug, Menuconfig, and size
     analysis from the ESP-IDF commands palette. Do not open the top-level repository
     folder as your active workspace while developing this example.
     ```

   - **Expected Serial Output** — representative log lines showing successful operation
   - **Key Concepts** — bullet list of the main ESP-IDF APIs and patterns demonstrated
   - **Spec Files** — links to `specs/FunctionalSpec.md` and `specs/CodingSpec.md`

9. Output: "Project generated. Build with `idf.py build` from `examples/<name>/`."
