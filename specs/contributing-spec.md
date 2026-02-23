# Contributing Specification

## SDD Contribution Rules (AI-first)

- Never edit generated files directly.
- Only edit files inside `/specs/` or `FunctionalSpec.md`, `CodingSpec.md`, `TestSpec.md` inside `examples/`.
- After any spec change: "Activate esp32-sdd-documentation-generator and rebuild all documentation."
- Submit PRs with spec changes only — generated files are regenerated, not hand-edited.

This keeps the repository 100% spec-driven.

---

## Commit Convention

All commits must follow the Conventional Commits format:

```
<type>(<scope>): <short summary>
```

**Types:**
- `feat` — new example, new skill, new spec section
- `fix` — correction to spec content or generated output
- `spec` — spec-only change (no generated output yet)
- `gen` — regenerated files following a spec change
- `chore` — maintenance (gitignore, tooling, non-spec files)
- `docs` — documentation corrections that do not involve spec changes

**Scope:** optional, use the folder name (e.g., `blink`, `skills`, `shared-specs`).

**Examples:**
```
feat(blink): add FunctionalSpec for LED blink example
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

---

## Code of Conduct

### Our Pledge

We as members, contributors, and maintainers pledge to make participation in this project a
harassment-free experience for everyone, regardless of age, body size, visible or invisible
disability, ethnicity, sex characteristics, gender identity and expression, level of experience,
education, socio-economic status, nationality, personal appearance, race, caste, colour, religion,
or sexual identity and orientation.

We pledge to act and interact in ways that contribute to an open, welcoming, diverse, inclusive,
and healthy community.

### Expected Behaviour

- Use welcoming and inclusive language.
- Be respectful of differing viewpoints and experiences.
- Accept constructive criticism gracefully.
- Focus on what is best for the community and the project.
- Show empathy towards other community members.
- Credit others' ideas, specs, and contributions appropriately.
- Keep discussions focused on technical merit and spec quality.

### Prohibited Behaviour

- Harassment, trolling, insulting, or derogatory comments.
- Personal or political attacks.
- Publishing others' private information without explicit permission.
- Sexual or violent language or imagery.
- Deliberate intimidation or sustained disruption of discussion.
- Any other conduct that would reasonably be considered inappropriate in a professional setting.

### Enforcement

Project maintainers are responsible for clarifying and enforcing these standards. Maintainers
have the right to remove, edit, or reject contributions that violate this code of conduct, and
to temporarily or permanently ban contributors for repeated or severe violations.

### Attribution

Adapted from the [Contributor Covenant v2.1](https://www.contributor-covenant.org/version/2/1/code_of_conduct/).

---

## Security Reporting

**Do not open a public GitHub Issue describing vulnerability details.**

To report a security concern:

1. Open a GitHub Issue and apply the `security` label.
2. In the issue body, state only: "I have found a security issue and would like to discuss it
   privately." Do not include any vulnerability details in the public issue.
3. A maintainer will acknowledge the issue within 48 hours and arrange private follow-up.

**Scope:** This project generates firmware source code and build configurations. Security reports
are relevant to the generation process itself (e.g., generated code that introduces common
vulnerabilities) rather than to ESP32 chip-level hardware security, which is out of scope.
