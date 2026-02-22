---
name: esp32-sdd-documentation-generator
description: Generates ALL Markdown files in the repo. Keeps root minimal and places extended docs in /docs/. Enforces 100% AI-first rule.
version: "1.0.0"
author: Rich Naszcyniec
keywords: sdd, esp32, documentation, agent-generated, docs-folder
---

# ESP32 SDD Showcase Documentation Generator

## When to use
Any time specs/ changes or you add examples/boards.

## Workflow (follow exactly)
1. Read all files in specs/
2. Generate minimal root README.md + CONTRIBUTING.md
3. Generate full docs/ folder + all example READMEs + board-specs/README.md + skills/README.md + etc.
4. Add the standard agent-generated header to every output file.
5. End with “All documentation is now in sync.”
