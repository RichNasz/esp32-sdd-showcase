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
