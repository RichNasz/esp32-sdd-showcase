# Documentation Generation Specification (AI-First Rule)

This file is the single source of truth for ALL Markdown documentation.

## Core Rules

- Human input strictly limited to `/specs/` folder.
- Generated files:
  - Root: README.md, CONTRIBUTING.md, CODE_OF_CONDUCT.md, SECURITY.md
  - GitHub templates: `.github/ISSUE_TEMPLATE/spec-improvement.md`, `.github/ISSUE_TEMPLATE/bug-report.md`, `.github/PULL_REQUEST_TEMPLATE.md`
  - Extended docs: `/docs/`, example READMEs, `board-specs/README.md`, `skills/README.md`, etc.
- Every generated `.md` file must carry the standard agent-generated header.
- Tone: professional, precise, enthusiastic, community-focused.
- Every generated file that describes or depicts example directory layout must match `specs/example-project-structure-spec.md` exactly.
- **NEVER generate files inside `.github/workflows/`** — GitHub restricts access to this directory without workflow scope.

---

## Root README.md Required Sections

The root `README.md` must contain the following 13 sections in this exact order:

| # | Section |
|---|---|
| 0 | Agent-generated header |
| 1 | Badge row |
| 2 | Title + tagline |
| 3 | 2–3 sentence project description (what + why + who) |
| 4 | Features: 4–6 bullets covering unique value props |
| 5 | Table of contents with jump links |
| 6 | Quick Start |
| 7 | Prerequisites (ESP-IDF v5.x, Claude Code, XIAO ESP32S3 or compatible) |
| 8 | Examples table (all examples: name + one-line description + link) |
| 9 | Skills table (all skills: skill name + purpose) |
| 10 | Full Documentation link table |
| 11 | Contributing (one paragraph + link to CONTRIBUTING.md) |
| 12 | License (MIT — see LICENSE) |
| 13 | Footer (generated-by credit) |

### Badge Row (exact markdown)

```markdown
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
![ESP-IDF](https://img.shields.io/badge/ESP--IDF-v5.x-blue)
![Chip](https://img.shields.io/badge/chip-ESP32--S3-orange)
![Built with Claude Code](https://img.shields.io/badge/Built%20with-Claude%20Code-blueviolet)
```

Do not add a CI/build-status badge — `.github/workflows/` is off-limits and a broken badge is
worse than no badge.

---

## New Generated Files

### CODE_OF_CONDUCT.md (root)

Based on Contributor Covenant v2.1 adapted for AI-first projects. Source content from
`specs/contributing-spec.md` (Code of Conduct section). Must include:
- Our Pledge
- Expected Behaviour
- Prohibited Behaviour
- Enforcement
- Reporting instructions: open a GitHub Issue with the `security` label (do not disclose
  vulnerability details publicly); maintainer will follow up privately within 48 hours.
- Attribution to Contributor Covenant v2.1.

### SECURITY.md (root)

Security policy document. Must include:
- Scope: firmware generation and build configurations; chip-level hardware security is out of scope.
- Reporting process: open a GitHub Issue with the `security` label, state only that a security
  issue exists (no public details), maintainer acknowledges within 48 hours and arranges private
  follow-up.
- What is in scope vs out of scope.
- Note: no coordinated disclosure timeline is guaranteed for this project stage.

### .github/ISSUE_TEMPLATE/spec-improvement.md

Guided form for spec change requests. Must include:
- YAML front matter: `name`, `about`, `labels: ["spec"]`, `assignees: ""`
- Sections: Which spec file, Current behaviour/content, Proposed change, Motivation/rationale,
  Checklist (human-authored spec only, no generated files edited, documentation-generator run
  after change).

### .github/ISSUE_TEMPLATE/bug-report.md

Guided form for reporting generated-code defects. Must include:
- YAML front matter: `name`, `about`, `labels: ["bug"]`, `assignees: ""`
- Sections: Which example, ESP-IDF version, Board, Describe the bug, Steps to reproduce,
  Expected behaviour, Actual behaviour, Build log (code block), Checklist (confirmed not a
  hand-edit of generated files).

### .github/PULL_REQUEST_TEMPLATE.md

PR checklist enforcing SDD rules. Must include:
- Summary section.
- Checklist items:
  - [ ] This PR contains a spec change (file in `specs/`, `shared-specs/`, or example
        `FunctionalSpec.md` / `CodingSpec.md` / `TestSpec.md`)
  - [ ] `esp32-sdd-documentation-generator` was run after the spec change
  - [ ] No agent-generated files were edited by hand
  - [ ] Commit messages follow the Conventional Commits format (see CONTRIBUTING.md)
  - [ ] I have read and agree to the Code of Conduct

---

## CodingSpec.md Standard

CodingSpec.md is high-level guidance — architecture, constraints, preferred libraries,
non-functional requirements, and gotchas. It must never contain code snippets, specific function
calls, or step-by-step implementation instructions. When generating or reviewing documentation
that describes CodingSpec.md, reflect this standard accurately.

---

## Testing Section Generation

Every example README.md must include a **Testing** section generated from `specs/TestSpec.md`.

### Testing Philosophy (enforce in every generated Testing section)

- Automated testing using native ESP-IDF tools is strongly preferred.
- Additional tools (Wokwi CLI, pytest-embedded) are only included when TestSpec.md provides
  explicit justification and documented setup steps.
- Manual testing steps are only included when no practical automated alternative exists.

### Generated Testing Section Structure

The Testing section must appear after "Key Concepts" and before "Spec Files". It must:

1. Open with the testing philosophy as a one-sentence note.
2. List **Automated Tests** first (subsection), with numbered steps. If none exist, omit the
   subsection rather than showing an empty list.
3. List **Manual Tests** second (subsection), clearly labelled "Manual (hardware required)".
   Only include if TestSpec.md specifies manual steps. Explain in a note why automation is not
   practical for each step.
4. Never mix automated and manual steps in the same numbered list.
5. Provide beginner-friendly wording — assume the reader has ESP-IDF installed but is not an expert.

### When TestSpec.md Is Absent

If `specs/TestSpec.md` does not exist for an example, omit the Testing section entirely from
the README. Do not generate a placeholder.

---

## Limitations Section Generation

When `specs/FunctionalSpec.md` includes a "Limitations and Topology Characterization"
section (or any section whose heading contains "Limitations"), the generated README.md
**must** include a corresponding **Limitations** section.

### Rules

- **Position**: immediately after "Key Concepts" and before "Testing" (or before
  "Spec Files" if Testing is absent).
- **Content**: reproduce the shortcomings and extension guidance from FunctionalSpec.md
  accurately. Never soften, omit, or reframe limitations to make the example appear
  more capable than it is.
- **Tone**: factual and constructive. State what the example does not do, then provide
  a concrete path to the capability if the reader needs it.
- **Tables**: preserve any "What this example is NOT" table and the extensions table
  from the FunctionalSpec verbatim in structure; adapt prose as needed for README style.
- **When absent**: if FunctionalSpec.md has no Limitations section, omit this section
  from the README entirely — do not generate a placeholder.

### Why this rule exists

Over-claiming example capabilities (e.g. calling a tree topology with partial recovery
a "self-healing mesh") misleads engineers evaluating the example for production use.
Documented limitations give readers an accurate mental model and a clear upgrade path
rather than discovering the gap at deployment time.

---

## Per-Board Behavior Section Generation

Every example README.md that supports more than one board must include a **Per-Board
Behavior** section. This section answers a different question than the supported boards
table: not *"what do I need to change?"* but *"what will I observe differently?"*

### When to include

Include this section whenever `specs/FunctionalSpec.md` documents two or more boards in
its Supported Boards section. If only one board is supported, omit the section entirely.

### Required position

The Per-Board Behavior section must appear immediately after the Supported Boards table
and before the Build & Flash section.

### Required content

The section must contain a table with one row per supported board and one column per
behavioral dimension that varies across boards. Only include columns where at least one
board differs from the default. Common dimensions to check (draw data from
`specs/FunctionalSpec.md` — do not invent values):

- **Serial monitor during deep sleep**: whether the monitor connection drops and
  reconnects (native USB CDC / Serial/JTAG boards) or stays connected (UART bridge
  boards). This is the most common surprise for users switching boards.
- **LED feedback**: color-capable (WS2812) vs on/off (simple GPIO), and what each
  state means (e.g. green = success, red = failure).
- **Sensor data scope**: whether the board's radio covers extra bands or frequencies
  compared to the default (e.g. dual-band WiFi scanning 5 GHz APs).
- **Deep sleep current floor**: the measured or datasheet deep sleep current, relevant
  for battery life planning.

Always source values from the FunctionalSpec.md Per-Board Behavior table. Never derive
them independently from board-spec files — the FunctionalSpec is the single source of
truth for this example's behavioral claims.

### Formatting rules

- Use spaced table separators `| --- | --- |`.
- Bold the default board row label or mark it *(default)*.
- Add a short paragraph after the table calling out the single most consequential
  behavioral difference for a first-time user (typically serial monitor dropout).
