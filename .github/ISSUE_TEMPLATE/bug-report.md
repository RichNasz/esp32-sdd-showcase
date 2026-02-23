---
name: Bug Report
about: Report a defect in agent-generated code or documentation
labels: ["bug"]
assignees: ""
---

<!-- ================================================
     AGENT-GENERATED — DO NOT EDIT BY HAND
     Generated from specs/ using esp32-sdd-documentation-generator skill
     Date: 2026-02-23 | Agent: Claude Code
     ================================================ -->

## Which example?

<!-- e.g. blinky, deep-sleep-timer-node -->

## ESP-IDF version

<!-- Run: idf.py --version -->

## Board

<!-- e.g. XIAO ESP32S3, Adafruit HUZZAH32 -->

## Describe the bug

<!-- A clear and concise description of what the bug is. -->

## Steps to reproduce

1.
2.
3.

## Expected behaviour

<!-- What did you expect to happen? -->

## Actual behaviour

<!-- What actually happened? -->

## Build log

```
<!-- Paste the relevant output from idf.py build or idf.py flash monitor here -->
```

## Checklist

- [ ] I confirmed that the file exhibiting the bug is agent-generated (has the agent-generated header comment)
- [ ] I have **not** manually edited the generated file before filing this report
- [ ] I have tried with a clean build directory (`rm -rf build/` then `idf.py build`)
