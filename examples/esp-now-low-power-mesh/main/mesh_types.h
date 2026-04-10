/* ================================================
   AGENT-GENERATED — DO NOT EDIT BY HAND
   Generated from specs/ using esp32-sdd-full-project-generator skill
   Date: 2026-04-08 | Agent: Claude Code
   ================================================ */

#pragma once

#include <stdint.h>

/** RTC memory magic constant — validates gateway MAC cache on warm reset. */
#define ESPNOW_RTC_MAGIC           0xA5B6C7D8UL

/** Number of (node_id, msg_count) entries in the relay dedup circular buffer. */
#define ESPNOW_DEDUP_TABLE_SIZE    16

/** How long sensor and relay wait for a gateway beacon on startup (ms). */
#define ESPNOW_DISCOVERY_TIMEOUT_MS 2000

/** How long the sensor waits for its send callback before declaring failure (ms). */
#define ESPNOW_SEND_TIMEOUT_MS     200

/** Gateway beacon broadcast interval (ms). */
#define ESPNOW_BEACON_INTERVAL_MS  5000

/** Node type values for espnow_beacon_t.node_type */
#define ESPNOW_NODE_GATEWAY  0
#define ESPNOW_NODE_RELAY    1

/**
 * @brief Beacon packet broadcast by gateway and relay nodes.
 *
 * Receive callbacks distinguish beacons from data payloads by data_len
 * (sizeof(espnow_beacon_t) == 2 vs sizeof(espnow_payload_t) == 8).
 * The node_type field lets sensors prefer a gateway beacon but fall back
 * to a relay beacon if the gateway is out of direct range.
 * Relays only broadcast after they have a valid gateway MAC — preventing
 * sensors from routing through a relay that is itself disconnected.
 */
typedef struct __attribute__((packed)) {
    uint8_t seq;       /**< Monotonically incrementing sequence number for diagnostics. */
    uint8_t node_type; /**< ESPNOW_NODE_GATEWAY or ESPNOW_NODE_RELAY. */
} espnow_beacon_t;

/**
 * @brief Data payload sent by sensor nodes and forwarded by relays.
 *
 * 8 bytes total — well within the 250-byte ESP-NOW maximum payload limit.
 */
typedef struct __attribute__((packed)) {
    uint8_t  node_id;    /**< Last byte of WiFi STA MAC (derived at runtime).    */
    uint16_t msg_count;  /**< Monotonic counter, stored in RTC_DATA_ATTR memory. */
    uint8_t  hop_count;  /**< 0 = direct to gateway, 1+ = relayed.              */
    int8_t   rssi[3];    /**< RSSI per configured SSID slot (dBm); INT8_MIN = not found. */
    uint8_t  role;       /**< Originating node role: 0=SENSOR, 1=SENSOR_RELAY.   */
} espnow_payload_t;

/** Role values for espnow_payload_t.role */
#define ESPNOW_ROLE_SENSOR        0
#define ESPNOW_ROLE_SENSOR_RELAY  1

_Static_assert(sizeof(espnow_payload_t) <= 250,
               "espnow_payload_t exceeds ESP-NOW 250-byte maximum");
