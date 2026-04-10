/* ================================================
   AGENT-GENERATED — DO NOT EDIT BY HAND
   Generated from specs/ using esp32-sdd-full-project-generator skill
   Date: 2026-04-08 | Agent: Claude Code
   ================================================ */

#pragma once

/**
 * @brief Start the gateway node.
 *
 * Initialises WiFi and ESP-NOW, starts the periodic beacon timer, registers the
 * receive callback for logging, then suspends indefinitely. Never returns.
 */
void gateway_run(void);
