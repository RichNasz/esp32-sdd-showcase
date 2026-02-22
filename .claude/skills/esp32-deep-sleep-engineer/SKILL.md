---
name: esp32-deep-sleep-engineer
description: Designs and generates deep-sleep configurations for ESP32. Handles ULP programs, RTC wake stubs, wakeup sources (timer, GPIO, touch, ULP), power-budget calculations, and RTC-safe peripheral teardown sequences.
version: "1.0.0"
author: Richard Naszcyniec
keywords: sdd, esp32, deep-sleep, ulp, rtc, low-power, wakeup, power-budget
---

<!-- ================================================
     AGENT-GENERATED — DO NOT EDIT BY HAND
     Generated from specs/ using esp32-sdd-documentation-generator skill
     Date: 2026-02-22 | Agent: Claude Code
     ================================================ -->

# ESP32 Deep Sleep Engineer

## When to use
Any time a FunctionalSpec requires deep sleep, ULP programs, or a sub-1 mA average current target. Invoked automatically as a sub-workflow by `esp32-sdd-full-project-generator` when sleep is specified.

## Workflow (follow exactly)
1. Read the FunctionalSpec.md wakeup strategy and hardware list.
2. Select wakeup source(s): timer, ext0, ext1, touch, ULP, or combination; justify the choice.
3. Calculate power budget: (active_mA × active_s + sleep_µA × sleep_s) / period_s = avg_µA.
4. Generate `main/deep_sleep.c` and `main/deep_sleep.h` with the correct `esp_sleep_enable_*` calls and `esp_deep_sleep_start()` flow.
5. Generate RTC wake stub (`RTC_IRAM_ATTR void esp_wake_deep_sleep(void)`) if sub-ms wakeup overhead is required.
6. Generate ULP assembly or ULP-FSM RISC-V program if the spec requires sensor polling during sleep.
7. Emit `sdkconfig.defaults` fragments for all relevant `CONFIG_ESP_SLEEP_*` options.
8. Document the wakeup sequence and power budget table in the example README.
