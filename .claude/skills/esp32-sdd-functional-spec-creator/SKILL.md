---
name: esp32-sdd-functional-spec-creator
description: Interactively guides creation of a FunctionalSpec.md for a new ESP32 example. Captures feature goals, hardware dependencies, wakeup strategy, connectivity model, and success criteria in the canonical SDD format consumed by esp32-sdd-full-project-generator.
version: "1.0.0"
author: Richard Naszcyniec
keywords: sdd, esp32, functional-spec, requirements, planning, specification
---

<!-- ================================================
     AGENT-GENERATED — DO NOT EDIT BY HAND
     Generated from specs/ using esp32-sdd-documentation-generator skill
     Date: 2026-02-22 | Agent: Claude Code
     ================================================ -->

# ESP32 SDD Functional Spec Creator

## When to use
When starting a new example or feature. Always run this skill before `esp32-sdd-full-project-generator` — the full-project generator requires a completed `FunctionalSpec.md` as its primary input.

## Workflow (follow exactly)
MANDATORY: First read and follow `shared-specs/AIGenLessonsLearned.md` before doing anything else.

1. Ask the user for: example name (slug), target board, and high-level goal in one sentence.
2. Derive hardware requirements from the goal (sensors, radios, power mode, security needs, connectivity).
3. Ask clarifying questions only if a requirement is genuinely ambiguous (wakeup source, MQTT topic, etc.).
4. Draft all `FunctionalSpec.md` sections: Overview, Target Hardware, Features, Wakeup Strategy (if applicable), Connectivity, Security Requirements, and Success Criteria.
5. Present the complete draft for user review; apply any corrections verbatim.
6. Write the approved spec to `examples/<name>/FunctionalSpec.md` with the standard agent-generated header.
7. Prompt: "Spec complete. Add a `CodingSpec.md` if needed, then activate `esp32-sdd-full-project-generator` to generate the project."
