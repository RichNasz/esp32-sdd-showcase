<!-- ================================================
     AGENT-GENERATED — DO NOT EDIT BY HAND
     Generated from specs/ using esp32-sdd-documentation-generator skill
     Date: 2026-02-22 | Agent: Claude Code
     ================================================ -->

# Contributing

## The Golden Rule

**Never edit agent-generated files directly.** If something looks wrong in a generated file, fix the spec that drives it and regenerate.

## What You Can Edit

| Path | You may edit? |
|---|---|
| `specs/*.md` | Yes — human-authored source of truth |
| `shared-specs/*.md` | Yes — platform-wide constraints |
| `examples/*/specs/FunctionalSpec.md` | Yes — per-example goals and requirements |
| `examples/*/specs/CodingSpec.md` | Yes — per-example implementation preferences |
| Everything else | No — agent-generated; regenerate instead |

## Workflow

1. Edit the relevant file in `specs/`, `shared-specs/`, or `examples/*/specs/`.
2. In Claude Code, say:
   > "Activate esp32-sdd-documentation-generator and rebuild all documentation."
3. Commit **both** the spec change and all regenerated files together:
   ```sh
   git add specs/ examples/*/specs/ <all regenerated files>
   git commit -m "spec+gen: <description of what changed>"
   ```

## Commit Types

| Prefix | When to use |
|---|---|
| `spec:` | Only spec files changed (human-authored) |
| `gen:` | Only agent-generated files changed |
| `spec+gen:` | Both spec and generated files changed together |

## Submitting a Pull Request

- PRs should contain spec changes as the driver — generated files are acceptable as companions.
- Never open a PR that edits generated files without a corresponding spec change.
- Every generated file must carry the standard agent-generated header comment.

For full details see [specs/contributing-spec.md](specs/contributing-spec.md).
