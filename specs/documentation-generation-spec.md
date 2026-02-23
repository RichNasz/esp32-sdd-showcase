# Documentation Generation Specification (AI-First Rule)

This file is the single source of truth for ALL Markdown documentation.

## Core Rules
- Human input strictly limited to /specs/ folder.
- Generated files:
  - Root: README.md, CONTRIBUTING.md (minimal only)
  - Everything else: /docs/, example READMEs, board-specs/README.md, etc.
- Every generated .md file must carry the standard agent-generated header.
- Root README.md must stay concise and link prominently to docs/README.md.
- Tone: professional, precise, enthusiastic, community-focused.
- Every generated file that describes or depicts example directory layout must match `specs/example-project-structure-spec.md` exactly.
- Never generate files inside `.github/workflows/` — GitHub restricts access to this directory without workflow scope.

## CodingSpec.md Standard

CodingSpec.md is high-level guidance — architecture, constraints, preferred libraries, non-functional requirements, and gotchas. It must never contain code snippets, specific function calls, or step-by-step implementation instructions. When generating or reviewing documentation that describes CodingSpec.md, reflect this standard accurately.

## Testing Section Generation

Every example README.md must include a **Testing** section generated from `specs/TestSpec.md`.

### Testing Philosophy (enforce in every generated Testing section)
- Automated testing using native ESP-IDF tools is strongly preferred.
- Additional tools (Wokwi CLI, pytest-embedded) are only included when TestSpec.md provides explicit justification and documented setup steps.
- Manual testing steps are only included when no practical automated alternative exists.

### Generated Testing Section Structure
The Testing section must appear after "Key Concepts" and before "Spec Files". It must:
1. Open with the testing philosophy as a one-sentence note.
2. List **Automated Tests** first (subsection), with numbered steps. If none exist, omit the subsection rather than showing an empty list.
3. List **Manual Tests** second (subsection), clearly labelled "Manual (hardware required)". Only include if TestSpec.md specifies manual steps. Explain in a note why automation is not practical for each step.
4. Never mix automated and manual steps in the same numbered list.
5. Provide beginner-friendly wording — assume the reader has ESP-IDF installed but is not an expert.

### When TestSpec.md Is Absent
If `specs/TestSpec.md` does not exist for an example, omit the Testing section entirely from the README. Do not generate a placeholder.
