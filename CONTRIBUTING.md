<!-- ================================================
     AGENT-GENERATED — DO NOT EDIT BY HAND
     Generated from specs/ using esp32-sdd-documentation-generator skill
     Date: 2026-02-23 | Agent: Claude Code
     ================================================ -->

# Contributing

## The Golden Rule

**Never edit agent-generated files directly.** If something looks wrong in a generated file,
fix the spec that drives it and regenerate.

## What You Can Edit

| Path | You may edit? |
|---|---|
| `specs/*.md` | Yes — human-authored source of truth |
| `shared-specs/*.md` | Yes — platform-wide constraints |
| `examples/*/specs/FunctionalSpec.md` | Yes — per-example goals and requirements |
| `examples/*/specs/CodingSpec.md` | Yes — per-example implementation guidance |
| `examples/*/specs/TestSpec.md` | Yes — per-example testing requirements |
| Everything else | No — agent-generated; regenerate instead |

## Workflow

1. Edit the relevant file in `specs/`, `shared-specs/`, or `examples/*/specs/`.
2. In Claude Code, say:
   > "Activate esp32-sdd-documentation-generator and rebuild all documentation."
3. Commit **both** the spec change and all regenerated files together.

## Commit Convention

All commits must follow [Conventional Commits](https://www.conventionalcommits.org/) format:

```
<type>(<scope>): <short summary>
```

**Types:**

| Type | When to use |
|---|---|
| `feat` | New example, new skill, new spec section |
| `fix` | Correction to spec content or generated output |
| `spec` | Spec-only change (no generated output yet) |
| `gen` | Regenerated files following a spec change |
| `chore` | Maintenance (gitignore, tooling, non-spec files) |
| `docs` | Documentation corrections that do not involve spec changes |

**Scope:** optional; use the folder name (e.g., `blink`, `skills`, `shared-specs`).

**Examples:**

```
feat(blinky): add FunctionalSpec for LED blink example
gen: regenerate docs after contributing-spec update
fix(shared-specs): correct ESP-IDF version reference to v5.4
chore: update .gitignore to exclude sdkconfig.old
```

Short summary: imperative mood, lowercase, no trailing period, ≤72 characters total.

**AI attribution trailer:**

For `gen:` and `spec+gen:` commits, append the following trailer so that AI involvement is
machine-readable in `git log` and searchable across the repository history:

```
Generated-by: Claude Code
```

Full example:

```
gen: regenerate CONTRIBUTING.md after contributing-spec update

Generated-by: Claude Code
```

This aligns with the OpenSSF Security-Focused Guide for AI Code Assistant Instructions
(Sep 2025) recommendation for machine-readable AI attribution in version history.

## Submitting a Pull Request

- PRs should contain a spec change as the driver — generated files are acceptable as companions.
- Never open a PR that edits generated files without a corresponding spec change.
- Every generated file must carry the standard agent-generated header comment.
- Use the PR template (`.github/PULL_REQUEST_TEMPLATE.md`) — it enforces the SDD checklist.

## Code of Conduct

This project follows the [Code of Conduct](CODE_OF_CONDUCT.md). By participating you agree to
uphold its standards.

## Security

To report a security concern, see [SECURITY.md](SECURITY.md).

For full details see [specs/contributing-spec.md](specs/contributing-spec.md).
