<!-- ================================================
     HUMAN-WRITTEN — GOLD STANDARD INSTITUTIONAL MEMORY
     ================================================ -->

# AIGenLessonsLearned.md — Gold Standard Lessons Learned

**MANDATORY FIRST STEP**: Every time you generate, regenerate, fix, or debug ANY code in this repository, read this file first.

## Core Rules (Never Violate)
- Humans only ever edit files inside any `specs/` folder.
- Never edit generated files (main/, CMakeLists.txt, sdkconfig.defaults, README.md, etc.) by hand.
- If something is wrong, improve the spec — never patch the generated code.
- Always respect the permanent example structure in specs/example-project-structure-spec.md
- Always reference board-specs/ files instead of hard-coding pins.

## Key Lessons Learned from Real AI Generation (Updated Feb 2026)
- Precise board references + board-specs/*.md dramatically reduce hallucinations.
- Explicit power budgets, wakeup sources, and peripheral constraints in FunctionalSpec.md are essential for ESP32.
- When switching boards (e.g. HUZZAH32 ↔ XIAO ESP32S3), change only the one-line board selector and regenerate.
- Always specify FreeRTOS task priorities, stack sizes, and queue usage explicitly.
- Include watchdog, error handling, and deep-sleep current targets in every spec.
- Sine-wave breathing, debouncing, and state machines work best when the algorithm is fully described in the CodingSpec.
- The first error is almost always a missing constraint in the spec, not a bad AI.
- **Kconfig.projbuild must live in a component directory** (e.g. `main/`), not the project
  root. ESP-IDF only auto-discovers Kconfig.projbuild inside component dirs. Placing it at
  the root silently drops all its symbols, causing #error guards and undeclared-identifier
  cascades at compile time. (Feb 2026)

## Required Workflow When Something Goes Wrong
1. Read this file.
2. Check shared-specs/CodingStandards.md and the relevant board-spec.
3. Improve the FunctionalSpec.md or CodingSpec.md.
4. Regenerate with the full-project-generator skill.
5. Never manually edit generated code.

## CodingSpec.md Must Be High-Level Guidance (Not Implementation Instructions)

CodingSpec.md describes **what** and **why** — never **how**. It must not contain code snippets, `#define` statements, specific API call sequences, or numbered step-by-step instructions. It should describe architecture, constraints, preferred libraries and the reasoning behind them, non-functional requirements, trade-off decisions, and known gotchas.

The guiding principle: "The model we use today is the worst model we will ever use." A high-level spec benefits from better future models. A low-level spec just duplicates code in prose and locks the implementation to the current generation's idioms.

If you find yourself writing a function call, a struct definition, or a numbered sequence of API calls in CodingSpec.md, stop and reframe it as an architectural constraint or a trade-off decision. (Feb 2026)

## Template for Adding New Lessons
When you discover something important:
- Add it here under "Key Lessons Learned"
- Date it
- Make it actionable for future generations

This file is the living quality guardrail and institutional memory of the entire esp32-sdd-showcase project.
