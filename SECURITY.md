<!-- ================================================
     AGENT-GENERATED — DO NOT EDIT BY HAND
     Generated from specs/contributing-spec.md using esp32-sdd-documentation-generator skill
     Date: 2026-02-23 | Agent: Claude Code
     ================================================ -->

# Security Policy

## Scope

This project generates firmware source code and ESP-IDF build configurations for ESP32
microcontrollers. Security reports are relevant to the **generation process** itself — for
example, if a generated file introduces a common vulnerability pattern (buffer overflow, weak
random seed, hard-coded credentials). Chip-level hardware security of the ESP32 (eFuse, flash
encryption at the silicon level, secure boot chain) is **out of scope** for this security policy.

## Reporting a Vulnerability

**Do not open a public GitHub Issue that describes vulnerability details.**

Follow these steps instead:

1. Open a GitHub Issue and apply the **`security`** label.
2. In the issue body, state only: _"I have found a security issue and would like to discuss it
   privately."_ Do not include any vulnerability details, proof-of-concept code, or reproduction
   steps in the public issue.
3. A maintainer will **acknowledge the issue within 48 hours** and arrange private follow-up
   (typically via GitHub's private vulnerability reporting or direct message).

## What Is In Scope

- Generated C code that introduces known vulnerability patterns (e.g., `strcpy` without bounds,
  predictable random seeds for cryptographic use, hard-coded default credentials).
- Skill workflows that systematically produce insecure configurations (e.g., disabled TLS
  verification, world-readable NVS partitions).
- Spec templates that guide users toward insecure design decisions without warning.

## What Is Out of Scope

- ESP32 chip-level hardware vulnerabilities (report to Espressif directly).
- Vulnerabilities in ESP-IDF itself (report to the
  [ESP-IDF security team](https://github.com/espressif/esp-idf/blob/master/SECURITY.md)).
- Issues in third-party libraries referenced by generated code.
- Theoretical vulnerabilities with no practical impact on generated output.

## Disclosure Timeline

This project does not guarantee a coordinated disclosure timeline at this stage. The maintainer
will work in good faith to acknowledge, investigate, and address valid reports as quickly as
possible given project resources.
