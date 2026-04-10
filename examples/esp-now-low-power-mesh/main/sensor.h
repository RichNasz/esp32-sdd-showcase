/* ================================================
   AGENT-GENERATED — DO NOT EDIT BY HAND
   Generated from specs/ using esp32-sdd-full-project-generator skill
   Date: 2026-04-08 | Agent: Claude Code
   ================================================ */

#pragma once

/**
 * @brief Execute the full sensor wake cycle.
 *
 * Flow: validate RTC cache → increment msg_count → WiFi init → identity log →
 * LED init → gateway discovery (if needed) → WiFi RSSI scan → ESP-NOW send →
 * teardown → deep sleep. Never returns; ends with esp_deep_sleep_start().
 */
void sensor_run(void);
