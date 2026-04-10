/* ================================================
   AGENT-GENERATED — DO NOT EDIT BY HAND
   Generated from specs/ using esp32-sdd-full-project-generator skill
   Date: 2026-04-08 | Agent: Claude Code
   ================================================ */

#include "relay.h"
#include "mesh_common.h"
#include "mesh_types.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_now.h"
#include "esp_timer.h"
#include "esp_task_wdt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <string.h>

static const char *TAG = "relay";

static const uint8_t BROADCAST_MAC[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

/* Dedup table: circular buffer of (node_id, msg_count) pairs. Resets on reboot. */
typedef struct {
    uint8_t  node_id;
    uint16_t msg_count;
} dedup_entry_t;

static dedup_entry_t s_dedup_table[ESPNOW_DEDUP_TABLE_SIZE];
static uint8_t       s_dedup_next = 0;

/* Next-hop MAC (gateway) discovered via beacon; all-zeros means not yet discovered. */
static uint8_t s_next_hop_mac[6];
static bool    s_next_hop_valid = false;
static bool    s_relay_beacon_started = false;

static SemaphoreHandle_t s_discovery_sem;
static TaskHandle_t      s_led_task_handle;

/* ---------------------------------------------------------------------------
 * Relay beacon timer callback — broadcasts only after gateway is known
 * ------------------------------------------------------------------------- */

static void relay_beacon_cb(void *arg)
{
    static uint8_t s_relay_seq = 0;
    espnow_beacon_t b = { .seq = ++s_relay_seq, .node_type = ESPNOW_NODE_RELAY };
    esp_now_send(BROADCAST_MAC, (uint8_t *)&b, sizeof(b));
}

static void start_relay_beacon(void)
{
    if (s_relay_beacon_started) return;
    s_relay_beacon_started = true;

    mesh_espnow_add_peer(BROADCAST_MAC);

    esp_timer_handle_t relay_beacon_timer;
    const esp_timer_create_args_t rbt_args = {
        .callback        = relay_beacon_cb,
        .arg             = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name            = "relay_beacon",
    };
    ESP_ERROR_CHECK(esp_timer_create(&rbt_args, &relay_beacon_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(relay_beacon_timer,
                                             ESPNOW_BEACON_INTERVAL_MS * 1000LL));
    ESP_LOGI(TAG, "Relay beacon started (advertising upstream path)");
}

/* ---------------------------------------------------------------------------
 * ESP-NOW receive callback
 * ------------------------------------------------------------------------- */

static void relay_recv_cb(const esp_now_recv_info_t *info,
                           const uint8_t *data, int data_len)
{
    if (data_len == (int)sizeof(espnow_beacon_t)) {
        const espnow_beacon_t *b = (const espnow_beacon_t *)data;
        if (b->node_type != ESPNOW_NODE_GATEWAY) {
            return; /* Ignore beacons from other relays. */
        }
        if (!s_next_hop_valid) {
            memcpy(s_next_hop_mac, info->src_addr, 6);
            s_next_hop_valid = true;
            mesh_espnow_add_peer(s_next_hop_mac);
            if (s_discovery_sem != NULL) {
                xSemaphoreGive(s_discovery_sem);
            }
            ESP_LOGI(TAG, "Gateway discovered: " MACSTR, MAC2STR(s_next_hop_mac));
        }
        /* Start relay beacon if not already started — late discovery path. */
        start_relay_beacon();
        return;
    }

    if (data_len != (int)sizeof(espnow_payload_t)) {
        return; /* Unknown packet size — ignore. */
    }

    if (!s_next_hop_valid) {
        return; /* Cannot forward without a known gateway. */
    }

    const espnow_payload_t *payload = (const espnow_payload_t *)data;

    /* Dedup check: scan the circular table for a matching (node_id, msg_count) pair. */
    for (int i = 0; i < ESPNOW_DEDUP_TABLE_SIZE; i++) {
        if (s_dedup_table[i].node_id   == payload->node_id &&
            s_dedup_table[i].msg_count == payload->msg_count) {
            return; /* Already forwarded — drop silently. */
        }
    }

    /* Record in dedup table. */
    s_dedup_table[s_dedup_next].node_id   = payload->node_id;
    s_dedup_table[s_dedup_next].msg_count = payload->msg_count;
    s_dedup_next = (s_dedup_next + 1) % ESPNOW_DEDUP_TABLE_SIZE;

    /* Forward with incremented hop count. */
    espnow_payload_t fwd = *payload;
    fwd.hop_count++;

    esp_err_t err = esp_now_send(s_next_hop_mac, (uint8_t *)&fwd, sizeof(fwd));
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Forward failed: %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, ">>> FWD node=0x%02X  seq=%u  hops=%u",
                 payload->node_id, payload->msg_count, fwd.hop_count);
        if (s_led_task_handle != NULL) {
            xTaskNotifyGive(s_led_task_handle);
        }
    }
}

/* ---------------------------------------------------------------------------
 * Entry point
 * ------------------------------------------------------------------------- */

void relay_run(void)
{
    mesh_wifi_init();
    mesh_led_init();

    uint8_t node_id = mesh_node_id_get();
    uint8_t mac[6];
    mesh_node_mac_get(mac);
    ESP_LOGI(TAG, "node_id=%u mac=" MACSTR " role=RELAY", node_id, MAC2STR(mac));

    mesh_espnow_set_channel(CONFIG_ESPNOW_CHANNEL);
    mesh_espnow_init();
    ESP_ERROR_CHECK(esp_now_register_recv_cb(relay_recv_cb));

    /* Wait for gateway beacon to populate the peer table before forwarding starts. */
    s_discovery_sem = xSemaphoreCreateBinary();
    bool found = (xSemaphoreTake(s_discovery_sem,
                                 pdMS_TO_TICKS(ESPNOW_DISCOVERY_TIMEOUT_MS)) == pdTRUE);
    vSemaphoreDelete(s_discovery_sem);
    s_discovery_sem = NULL;

    if (found) {
        /* Gateway found within timeout — start advertising ourselves as a relay. */
        start_relay_beacon();
    } else {
        /* Continue in receive mode — recv_cb will start the beacon on next gateway beacon. */
        ESP_LOGW(TAG, "No gateway found within %d ms -- continuing in receive mode",
                 ESPNOW_DISCOVERY_TIMEOUT_MS);
    }

    ESP_ERROR_CHECK(esp_task_wdt_add(NULL));

    xTaskCreate(mesh_led_task, "mesh_led", CONFIG_MESH_LED_TASK_STACK_SIZE,
                NULL, 2, &s_led_task_handle);

    /* Steady-state: feed the task watchdog while the recv_cb handles all work. */
    while (1) {
        esp_task_wdt_reset();
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
