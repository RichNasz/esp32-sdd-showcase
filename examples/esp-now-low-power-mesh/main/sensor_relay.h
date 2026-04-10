/* ================================================
   AGENT-GENERATED — DO NOT EDIT BY HAND
   Generated from specs/ using esp32-sdd-full-project-generator skill
   Date: 2026-04-08 | Agent: Claude Code
   ================================================ */

#pragma once

/**
 * @brief Entry point for the SENSOR_RELAY role.
 *
 * Initialises WiFi, ESP-NOW, and power management light sleep. Runs a
 * relay recv_cb continuously and fires a periodic timer to run WiFi RSSI
 * scans and transmit the node's own payload. Never returns.
 */
void sensor_relay_run(void);
