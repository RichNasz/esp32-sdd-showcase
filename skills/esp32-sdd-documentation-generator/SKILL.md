---
name: esp32-sdd-documentation-generator
description: Generates ALL Markdown files in the repo. Keeps root minimal and places extended docs in /docs/. Enforces 100% AI-first rule.
version: "1.0.0"
author: Richard Naszcyniec
keywords: sdd, esp32, documentation, agent-generated, docs-folder
---

# ESP32 SDD Showcase Documentation Generator

## When to use
Any time specs/ changes or you add examples/boards.

## Workflow (follow exactly)
MANDATORY: First read and follow `shared-specs/AIGenLessonsLearned.md` before doing anything else.

1. Read all files in specs/ (including the updated example-project-structure-spec.md and documentation-generation-spec.md).
2. Generate minimal root README.md + CONTRIBUTING.md.
3. Generate full docs/ folder + all example READMEs + board-specs/README.md + skills/README.md + etc.
   - NEVER generate or place files inside `.github/workflows/` — this directory is off-limits.
4. For each example README.md, check whether `examples/<name>/specs/TestSpec.md` exists.
   - If it exists: generate a **Testing** section after “Key Concepts” and before “Spec Files”:
     - Open with one sentence: “Testing philosophy: automated first, manual only when hardware observation is unavoidable.”
     - **Automated Tests** subsection: numbered steps, native ESP-IDF tools first (idf.py build, Unity, Wokwi CLI). Include additional tools only if TestSpec.md documents them with justification.
     - **Manual Tests** subsection (label: “Manual — hardware required”): numbered steps, only for checks that cannot be automated. Each step must note why automation is not practical.
     - Never mix automated and manual steps in the same list.
     - Steps must be beginner-friendly (assume ESP-IDF installed, not an expert).
   - If TestSpec.md does not exist: omit the Testing section entirely. Do not generate a placeholder.
5. Add the standard agent-generated header to every output file.
6. End with “All documentation is now in sync.”
