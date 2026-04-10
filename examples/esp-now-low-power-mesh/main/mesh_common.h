/* ================================================
   AGENT-GENERATED — DO NOT EDIT BY HAND
   Generated from specs/ using esp32-sdd-full-project-generator skill
   Date: 2026-04-08 | Agent: Claude Code
   ================================================ */

#pragma once

#include <stdint.h>

/**
 * @brief Initialise NVS, netif, event loop, WiFi STA mode, and start the radio.
 *
 * Must be called once at startup before any WiFi or ESP-NOW operation.
 * Node identity (MAC) is available after this call.
 */
void mesh_wifi_init(void);

/**
 * @brief Stop and deinitialise the WiFi driver.
 *
 * Called by the sensor before entering deep sleep. Relay and gateway never call this.
 */
void mesh_wifi_deinit(void);

/**
 * @brief Initialise the ESP-NOW protocol stack.
 *
 * WiFi must already be running. Does not register any callbacks — each role registers
 * its own send/receive callbacks after calling this function.
 */
void mesh_espnow_init(void);

/**
 * @brief Deinitialise the ESP-NOW protocol stack and free its resources.
 */
void mesh_espnow_deinit(void);

/**
 * @brief Set the WiFi channel used for ESP-NOW communication.
 *
 * Must be called after WiFi start and before any ESP-NOW send or receive operation.
 * Resets to the fixed channel after a WiFi scan that may have left the radio on
 * a different channel.
 *
 * @param channel WiFi channel number (1–13).
 */
void mesh_espnow_set_channel(uint8_t channel);

/**
 * @brief Register a MAC address as an ESP-NOW peer.
 *
 * Uses CONFIG_ESPNOW_CHANNEL and WIFI_IF_STA. Safe to call if the peer is already
 * registered — duplicate registrations are silently ignored.
 *
 * @param mac 6-byte MAC address to register.
 */
void mesh_espnow_add_peer(const uint8_t *mac);

/**
 * @brief Derive a unique node identifier from the WiFi STA MAC address.
 *
 * Returns mac[5] (last byte of eFuse MAC). Falls back to mac[4] if mac[5] is 0x00
 * or 0xFF (pathological values that should never occur on production silicon).
 *
 * @return 8-bit node identifier, unique per device.
 */
uint8_t mesh_node_id_get(void);

/**
 * @brief Copy the full WiFi STA MAC address into the provided buffer.
 *
 * @param mac_out Caller-supplied 6-byte buffer.
 */
void mesh_node_mac_get(uint8_t *mac_out);

/**
 * @brief Configure the LED GPIO and set it to the off state.
 *
 * Calls gpio_hold_dis() before configuring so that pad state held from a previous
 * deep sleep cycle does not prevent the GPIO from being driven.
 */
void mesh_led_init(void);

/** @brief Drive the LED on. */
void mesh_led_on(void);

/** @brief Drive the LED off. */
void mesh_led_off(void);

/**
 * @brief Blink the LED once (20 ms on, then off).
 *
 * Blocks the calling task for 20 ms. Suitable for the sensor's linear app_main flow.
 * Relay and gateway use mesh_led_task instead.
 */
void mesh_led_blink_once(void);

/**
 * @brief Fast-blink the LED three times (100 ms on / 100 ms off per cycle).
 *
 * Used by the sensor to signal a send failure before entering deep sleep.
 */
void mesh_led_fast_blink(void);

/**
 * @brief FreeRTOS task that blinks the LED once per task notification received.
 *
 * Waits indefinitely on ulTaskNotifyTake(). Each xTaskNotifyGive() from a receive
 * callback produces one blink. Priority 2 (above app_main's priority 1) ensures
 * blink responsiveness regardless of other activity.
 *
 * @param arg Unused.
 */
void mesh_led_task(void *arg);
