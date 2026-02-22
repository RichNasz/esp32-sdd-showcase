<!-- ================================================
     AGENT-GENERATED — DO NOT EDIT BY HAND
     Generated from specs/ using esp32-sdd-documentation-generator skill
     Date: 2026-02-22 | Agent: Claude Code
     ================================================ -->

# SDD Workflows

The complete Spec-Driven Development workflows used in this project. Every workflow starts with a human-written spec and ends with agent-generated, buildable output — no manual code authoring.

## Core Principle

```
Human writes specs  →  AI reads specs  →  AI generates everything else
```

Humans are responsible for **what** and **why**. AI is responsible for **how**.

---

## Workflow 1 — Generate a New Example

Use this when you want to add a new ESP32 example to the catalog.

```
1. Edit specs/showcase-master-spec.md   (add your example to the catalog)
2. Say: "Activate esp32-sdd-functional-spec-creator skill"
        → Creates examples/<name>/FunctionalSpec.md
3. Review and approve the FunctionalSpec.md (you may annotate but not rewrite)
4. Add examples/<name>/CodingSpec.md    (optional: implementation preferences)
5. Say: "Activate esp32-sdd-full-project-generator skill"
        → Generates the complete ESP-IDF project
6. idf.py build flash monitor
```

---

## Workflow 2 — Add a Board Spec

Use this when adding support for a new development board or ESP32 module.

```
1. Provide the vendor product page URL or datasheet
2. Say: "Activate esp32-board-spec-generator skill"
        → Creates board-specs/<vendor>/<board-name>.md
3. Reference the new board spec in your example's FunctionalSpec.md
```

---

## Workflow 3 — Regenerate Documentation

Use this after any change to `specs/` or any `shared-specs/` file.

```
1. Edit the relevant file in specs/ or shared-specs/
2. Say: "Activate esp32-sdd-documentation-generator and rebuild all documentation."
        → Regenerates README.md, docs/, skills/README.md, etc.
3. Commit: specs/ changes + all regenerated files in the same commit
```

---

## Workflow 4 — Update an Existing Example

Use this when a FunctionalSpec or CodingSpec changes and the generated project needs to be rebuilt.

```
1. Edit examples/<name>/FunctionalSpec.md or CodingSpec.md
2. Say: "Activate esp32-sdd-full-project-generator skill"
        → Overwrites all generated files in examples/<name>/
        → Preserves FunctionalSpec.md and CodingSpec.md unchanged
3. Review the diff, build, and test
```

---

## Skill Dependency Map

```
esp32-sdd-functional-spec-creator
  │
  └─► esp32-sdd-full-project-generator
        │
        ├─► esp32-deep-sleep-engineer        (auto-invoked if sleep required)
        │
        └─► esp32-hardware-crypto-configurer (auto-invoked if security required)

esp32-board-spec-generator                   (independent, run on demand)

esp32-sdd-documentation-generator            (independent, run after spec changes)
```

---

## Git Commit Convention

Every commit should be one of:

| Type | Contents |
|---|---|
| `spec:` | Only changes to `specs/` or `shared-specs/` (human-authored) |
| `gen:` | Only changes to agent-generated files (triggered by a spec change) |
| `spec+gen:` | Both spec and generated files in one atomic update |

Never commit generated files without the driving spec change in the same or prior commit.

---

## Why SDD?

- **Reproducibility** — specs are the single source of truth; regeneration produces identical output
- **Reviewability** — PRs contain spec changes that are human-readable and auditable
- **Scalability** — adding a new example or board is a spec edit + skill invocation, not a coding session
- **Quality** — every generated file is consistent with every other; no drift between docs and code
