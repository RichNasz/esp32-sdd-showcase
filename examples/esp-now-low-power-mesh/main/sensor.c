/* ================================================
   AGENT-GENERATED — DO NOT EDIT BY HAND
   Generated from specs/ using esp32-sdd-full-project-generator skill
   Date: 2026-04-08 | Agent: Claude Code
   ================================================ */

#include "sensor.h"
#include "mesh_common.h"
#include "mesh_types.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_now.h"
#include "esp_sleep.h"
#include "esp_task_wdt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <string.h>
#include <limits.h>

static const char *TAG = "sensor";

/* RTC-retained state survives deep sleep; validated by magic constant on wake. */
static RTC_DATA_ATTR uint32_t s_rtc_magic;
static RTC_DATA_ATTR uint16_t s_msg_count;
static RTC_DATA_ATTR uint8_t  s_next_hop_mac[6];
static RTC_DATA_ATTR uint8_t  s_next_hop_type; /* ESPNOW_NODE_GATEWAY or ESPNOW_NODE_RELAY */

/* Semaphores for synchronising with ESP-NOW callbacks. */
static SemaphoreHandle_t s_send_sem;
static SemaphoreHandle_t s_beacon_sem;
static esp_now_send_status_t s_send_status;

/* Non-RTC: relay candidate MAC saved during discovery window as fallback. */
static uint8_t s_relay_candidate[6];

/* Static buffer for WiFi scan results — avoids stack overflow from large arrays. */
static wifi_ap_record_t s_scan_records[20];

/* ---------------------------------------------------------------------------
 * ESP-NOW callbacks
 * ------------------------------------------------------------------------- */

/* ESP-IDF 5.5+ send callback signature uses esp_now_send_info_t (wifi_tx_info_t). */
static void send_cb(const esp_now_send_info_t *tx_info, esp_now_send_status_t status)
{
    (void)tx_info;
    s_send_status = status;
    xSemaphoreGive(s_send_sem);
}

static void discovery_recv_cb(const esp_now_recv_info_t *info,
                               const uint8_t *data, int data_len)
{
    if (data_len != (int)sizeof(espnow_beacon_t)) return;
    const espnow_beacon_t *b = (const espnow_beacon_t *)data;
    if (b->node_type == ESPNOW_NODE_GATEWAY) {
        /* Gateway beacon — record as next hop and unblock immediately. */
        memcpy(s_next_hop_mac, info->src_addr, 6);
        s_next_hop_type = ESPNOW_NODE_GATEWAY;
        xSemaphoreGive(s_beacon_sem);
    } else if (b->node_type == ESPNOW_NODE_RELAY) {
        /* Relay beacon — save as fallback, keep waiting for gateway. */
        memcpy(s_relay_candidate, info->src_addr, 6);
    }
}

/* ---------------------------------------------------------------------------
 * WiFi RSSI scan
 * ------------------------------------------------------------------------- */

static void sensor_scan_rssi(int8_t *rssi_out)
{
    for (int i = 0; i < 3; i++) {
        rssi_out[i] = INT8_MIN;
    }

    const char *target_ssids[3] = {
        CONFIG_TARGET_SSID_1,
        CONFIG_TARGET_SSID_2,
        CONFIG_TARGET_SSID_3,
    };

    bool any_configured = false;
    for (int i = 0; i < 3; i++) {
        if (target_ssids[i][0] != '\0') {
            any_configured = true;
            break;
        }
    }

    if (!any_configured) {
        ESP_LOGW(TAG, "No target SSIDs configured — all RSSI slots will be INT8_MIN");
        return;
    }

    wifi_scan_config_t scan_cfg = {
        .channel     = 0,     /* 0 = scan all channels */
        .show_hidden = false,
        .scan_type   = WIFI_SCAN_TYPE_ACTIVE,
        .scan_time   = {
            .active = { .min = 50, .max = 100 },
        },
    };

    esp_err_t err = esp_wifi_scan_start(&scan_cfg, true); /* blocking */
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "WiFi scan start failed: %s", esp_err_to_name(err));
        return;
    }

    uint16_t ap_count = 0;
    esp_wifi_scan_get_ap_num(&ap_count);

    if (ap_count == 0) {
        ESP_LOGW(TAG, "WiFi scan: no APs found");
        /* Explicit stop per CodingSpec — radio must be idle before ESP-NOW init. */
        esp_wifi_scan_stop();
        return;
    }

    uint16_t max_records = (ap_count > 20) ? 20 : ap_count;
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&max_records, s_scan_records));

    /* Explicit stop; no-op on a completed blocking scan but required by spec. */
    esp_wifi_scan_stop();

    /* Match each AP record against configured SSIDs; keep highest RSSI per SSID. */
    for (uint16_t j = 0; j < max_records; j++) {
        for (int i = 0; i < 3; i++) {
            if (target_ssids[i][0] == '\0') continue;
            if (strcmp((char *)s_scan_records[j].ssid, target_ssids[i]) == 0) {
                if (s_scan_records[j].rssi > rssi_out[i]) {
                    rssi_out[i] = s_scan_records[j].rssi;
                }
            }
        }
    }

    /* Per-SSID output — show names alongside RSSI for demo readability. */
    ESP_LOGI(TAG, "WiFi scan complete:");
    for (int i = 0; i < 3; i++) {
        if (target_ssids[i][0] == '\0') continue;
        if (rssi_out[i] == INT8_MIN) {
            ESP_LOGI(TAG, "  %-24s  (not found)", target_ssids[i]);
        } else {
            ESP_LOGI(TAG, "  %-24s  %d dBm", target_ssids[i], rssi_out[i]);
        }
    }
}

/* ---------------------------------------------------------------------------
 * Main sensor wake cycle
 * ------------------------------------------------------------------------- */

void sensor_run(void)
{
    /* Validate RTC cache with magic constant. On power-on reset (first boot),
     * the magic will not match and all cached state is reset. On warm reset
     * (wake from deep sleep), the magic matches and the cache is reused. */
    if (s_rtc_magic != ESPNOW_RTC_MAGIC) {
        s_rtc_magic      = ESPNOW_RTC_MAGIC;
        s_msg_count      = 0;
        s_next_hop_type  = ESPNOW_NODE_GATEWAY;
        memset(s_next_hop_mac, 0, 6);
    }

    s_msg_count++;
    /* Wake banner — visually distinct for demo audiences watching the monitor. */
    ESP_LOGI(TAG, "========== WAKE #%u | node 0x%02X ==========",
             s_msg_count, mesh_node_id_get());

    mesh_wifi_init();
    mesh_led_init();

    uint8_t node_id = mesh_node_id_get();
    uint8_t mac[6];
    mesh_node_mac_get(mac);
    ESP_LOGI(TAG, "node=0x%02X  mac=" MACSTR "  role=SENSOR", node_id, MAC2STR(mac));

    /* ---- Next-hop discovery (only when cache is empty) ---- */
    bool next_hop_cached = false;
    for (int i = 0; i < 6; i++) {
        if (s_next_hop_mac[i] != 0) { next_hop_cached = true; break; }
    }

    if (!next_hop_cached) {
        memset(s_relay_candidate, 0, 6);

        mesh_espnow_set_channel(CONFIG_ESPNOW_CHANNEL);
        mesh_espnow_init();

        s_beacon_sem = xSemaphoreCreateBinary();
        ESP_ERROR_CHECK(esp_now_register_recv_cb(discovery_recv_cb));

        /* Wait the full timeout hoping for a gateway beacon.
         * If a gateway beacon arrives early, recv_cb gives the semaphore
         * immediately; a relay beacon is just saved as a fallback candidate. */
        bool gw_found = (xSemaphoreTake(s_beacon_sem,
                                        pdMS_TO_TICKS(ESPNOW_DISCOVERY_TIMEOUT_MS)) == pdTRUE);

        esp_now_unregister_recv_cb();
        mesh_espnow_deinit();
        vSemaphoreDelete(s_beacon_sem);
        s_beacon_sem = NULL;

        if (!gw_found) {
            /* Check if a relay beacon was received as fallback. */
            bool relay_found = false;
            for (int i = 0; i < 6; i++) {
                if (s_relay_candidate[i] != 0) { relay_found = true; break; }
            }
            if (relay_found) {
                memcpy(s_next_hop_mac, s_relay_candidate, 6);
                s_next_hop_type = ESPNOW_NODE_RELAY;
                ESP_LOGI(TAG, "No gateway — routing via relay " MACSTR,
                         MAC2STR(s_next_hop_mac));
            } else {
                ESP_LOGW(TAG, "No gateway or relay found within %d ms -- returning to sleep",
                         ESPNOW_DISCOVERY_TIMEOUT_MS);
                mesh_wifi_deinit();
                goto enter_sleep;
            }
        } else {
            ESP_LOGI(TAG, "Gateway discovered: " MACSTR, MAC2STR(s_next_hop_mac));
        }
    } else {
        const char *type_str = (s_next_hop_type == ESPNOW_NODE_RELAY) ? "relay" : "gateway";
        ESP_LOGI(TAG, "Next hop cached (%s): " MACSTR, type_str, MAC2STR(s_next_hop_mac));
    }

    /* ---- WiFi RSSI scan ---- */
    int8_t rssi[3];
    sensor_scan_rssi(rssi);

    /* Reset channel after scan — the scan driver may have left the radio on a
     * different channel. Must be set before ESP-NOW init. */
    mesh_espnow_set_channel(CONFIG_ESPNOW_CHANNEL);

    /* ---- ESP-NOW send ---- */
    mesh_espnow_init();
    mesh_espnow_add_peer(s_next_hop_mac);

    s_send_sem = xSemaphoreCreateBinary();
    ESP_ERROR_CHECK(esp_now_register_send_cb(send_cb));

    espnow_payload_t payload = {
        .node_id   = node_id,
        .msg_count = s_msg_count,
        .hop_count = 0,
        .rssi      = { rssi[0], rssi[1], rssi[2] },
        .role      = ESPNOW_ROLE_SENSOR,
    };

    const char *dest_str = (s_next_hop_type == ESPNOW_NODE_RELAY) ? "via relay" : "to gateway";
    esp_err_t err = esp_now_send(s_next_hop_mac, (uint8_t *)&payload, sizeof(payload));
    if (err != ESP_OK) {
        /* esp_now_send failure is recoverable — log and clear cached next-hop MAC. */
        ESP_LOGE(TAG, "esp_now_send error: %s", esp_err_to_name(err));
        memset(s_next_hop_mac, 0, 6);
        mesh_led_fast_blink();
    } else {
        bool cb_received = (xSemaphoreTake(s_send_sem,
                                           pdMS_TO_TICKS(ESPNOW_SEND_TIMEOUT_MS)) == pdTRUE);
        if (cb_received && s_send_status == ESP_NOW_SEND_SUCCESS) {
            ESP_LOGI(TAG, "Sending %s... OK", dest_str);
            mesh_led_blink_once();
        } else {
            const char *reason = cb_received ? "send callback: FAIL" : "send callback: timeout";
            ESP_LOGW(TAG, "%s -- invalidating next-hop MAC cache", reason);
            memset(s_next_hop_mac, 0, 6);
            mesh_led_fast_blink();
        }
    }

    vSemaphoreDelete(s_send_sem);
    s_send_sem = NULL;

    mesh_espnow_deinit();
    mesh_wifi_deinit();

enter_sleep:
    /* Delete ourselves from the task watchdog before sleeping — the watchdog
     * cannot be fed from deep sleep, so we must remove the subscription. */
    esp_task_wdt_delete(NULL);

    ESP_LOGI(TAG, "========== Sleeping %d s ==========", CONFIG_SENSOR_SLEEP_SEC);
    esp_sleep_enable_timer_wakeup((uint64_t)CONFIG_SENSOR_SLEEP_SEC * 1000000ULL);
    esp_deep_sleep_start();
}
