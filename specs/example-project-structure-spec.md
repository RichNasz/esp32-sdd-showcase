<!-- ================================================
     HUMAN-WRITTEN — SINGLE SOURCE OF TRUTH — PERMANENT
     ================================================ -->

# Example Project Structure Specification (PERMANENT — NEVER CHANGE)

This file defines the official, immutable structure for every example in the repository.

## The Locked Structure

```
examples/<kebab-name>/
├── specs/                          ← Human-only (FunctionalSpec.md + CodingSpec.md + TestSpec.md)
│   ├── FunctionalSpec.md
│   ├── CodingSpec.md
│   └── TestSpec.md                 ← NEW: Human-written testing requirements
├── main/                           ← Official ESP-IDF main component (100% agent-generated)
│   ├── main.c
│   ├── CMakeLists.txt
│   └── Kconfig.projbuild           ← Optional; required for multi-board Kconfig choices
├── CMakeLists.txt                  ← Root CMakeLists.txt (generated)
├── sdkconfig.defaults              ← ESP-IDF config (generated)
├── idf_component.yml               ← Dependencies (generated when needed)
├── README.md                       ← Generated
├── .gitignore                      ← Generated
└── build/                          ← gitignored
```

## CodingSpec.md — Abstraction Level

CodingSpec.md is high-level technical guidance for the AI model generating the project. It must describe **what** the system should do and **why** — not **how** to implement it.

**Include in CodingSpec.md:**
- Architecture and component breakdown
- Key timing, memory, power, and security constraints
- Preferred libraries and APIs (and the reason for each choice)
- Non-functional requirements (error recovery philosophy, logging expectations, thread safety)
- Trade-off decisions (e.g., QoS 0 vs 1, polling vs interrupt-driven)
- Known gotchas specific to the hardware or ESP-IDF version

**Never include in CodingSpec.md:**
- Code snippets, `#define` statements, or pseudo-code
- Specific function signatures or API call sequences
- Exact log message strings
- Numbered step-by-step implementation instructions

**Philosophy:** "The model we use today is the worst model we will ever use." A high-level spec will benefit from better future models. A low-level spec just replicates code in prose.

## TestSpec.md — Testing Philosophy

TestSpec.md is the third human-written spec file. It defines testing requirements for the example.

**Philosophy (enforced in every TestSpec.md):**
> Automated testing using native ESP-IDF tools is strongly preferred. Additional tools are only allowed with explicit justification and documented benefits. Manual testing is used only when no practical automated alternative exists.

**Tiers (in priority order):**
1. **Automated — native ESP-IDF**: `idf.py build` (zero-warning check), Unity test components built with the project, Wokwi simulation via `wokwi-cli`. No extra tooling required beyond the standard ESP-IDF installation.
2. **Automated — additional tools**: Wokwi CI integration, Host-side pytest via `pytest-embedded`. Allowed only with explicit justification and documented setup steps in TestSpec.md.
3. **Manual — hardware-only**: Visual or physical observation that cannot be simulated (LED polarity, haptics, RF behaviour). Required steps must be numbered and beginner-friendly.

This structure is permanent and must be strictly enforced by the full-project-generator skill in every example, now and forever.
