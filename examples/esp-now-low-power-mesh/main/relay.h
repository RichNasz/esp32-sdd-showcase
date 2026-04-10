/* ================================================
   AGENT-GENERATED — DO NOT EDIT BY HAND
   Generated from specs/ using esp32-sdd-full-project-generator skill
   Date: 2026-04-08 | Agent: Claude Code
   ================================================ */

#pragma once

/**
 * @brief Start the relay node.
 *
 * Initialises WiFi and ESP-NOW, discovers the gateway via beacon, then suspends
 * indefinitely while the ESP-NOW receive callback handles all forwarding.
 * Never returns.
 */
void relay_run(void);
