<!-- ================================================
     HUMAN-WRITTEN — SINGLE SOURCE OF TRUTH — PERMANENT
     ================================================ -->

# Example Project Structure Specification (PERMANENT — NEVER CHANGE)

This file defines the official, immutable structure for every example in the repository.

## The Locked Structure

```
examples/<kebab-name>/
├── specs/                          ← Human-only (FunctionalSpec.md + CodingSpec.md)
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

This structure is permanent and must be strictly enforced by the full-project-generator skill in every example, now and forever.
