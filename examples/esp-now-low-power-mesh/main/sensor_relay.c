/* ================================================
   AGENT-GENERATED — DO NOT EDIT BY HAND
   Generated from specs/ using esp32-sdd-full-project-generator skill
   Date: 2026-04-08 | Agent: Claude Code
   ================================================ */

#include "sensor_relay.h"
#include "mesh_common.h"
#include "mesh_types.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_now.h"
#include "esp_timer.h"
#include "esp_task_wdt.h"
#include "esp_pm.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <string.h>
#include <limits.h>

static const char *TAG = "sr";  /* sensor-relay */

static const uint8_t BROADCAST_MAC[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

/* Static state — no deep sleep in this role, so plain static suffices. */
static uint16_t  s_msg_count      = 0;
static uint8_t   s_next_hop_mac[6];
static bool      s_next_hop_valid  = false;
static bool      s_relay_beacon_started = false;

/* Relay beacon timer handle kept module-level so it can be stopped on gateway loss. */
static esp_timer_handle_t s_relay_beacon_timer = NULL;

/* Send semaphore + status for the scan cycle — mirrors the pattern in sensor.c. */
static SemaphoreHandle_t     s_send_sem;
static esp_now_send_status_t s_send_status;

/* Semaphores and task handle. */
static SemaphoreHandle_t s_discovery_sem;
static SemaphoreHandle_t s_scan_sem;
static TaskHandle_t      s_led_task_handle;

/* Static WiFi scan buffer — avoids large stack allocation in app_main. */
static wifi_ap_record_t s_scan_records[20];

/* Dedup table: circular buffer of (node_id, msg_count) pairs. */
typedef struct {
    uint8_t  node_id;
    uint16_t msg_count;
} dedup_entry_t;

static dedup_entry_t s_dedup_table[ESPNOW_DEDUP_TABLE_SIZE];
static uint8_t       s_dedup_next = 0;

/* ---------------------------------------------------------------------------
 * Relay beacon timer callback — broadcasts only after gateway is known
 * ------------------------------------------------------------------------- */

static void sr_send_cb(const esp_now_send_info_t *tx_info, esp_now_send_status_t status)
{
    (void)tx_info;
    s_send_status = status;
    xSemaphoreGive(s_send_sem);
}

static void relay_beacon_cb(void *arg)
{
    static uint8_t s_relay_seq = 0;
    espnow_beacon_t b = { .seq = ++s_relay_seq, .node_type = ESPNOW_NODE_RELAY };
    esp_now_send(BROADCAST_MAC, (uint8_t *)&b, sizeof(b));
}

/* Start (or restart after a stop) the relay beacon timer.
 * The timer handle is created once and reused across gateway loss/rediscovery cycles. */
static void start_relay_beacon(void)
{
    if (s_relay_beacon_started) return;
    s_relay_beacon_started = true;

    mesh_espnow_add_peer(BROADCAST_MAC);

    if (s_relay_beacon_timer == NULL) {
        const esp_timer_create_args_t rbt_args = {
            .callback        = relay_beacon_cb,
            .arg             = NULL,
            .dispatch_method = ESP_TIMER_TASK,
            .name            = "sr_relay_beacon",
        };
        ESP_ERROR_CHECK(esp_timer_create(&rbt_args, &s_relay_beacon_timer));
    }
    ESP_ERROR_CHECK(esp_timer_start_periodic(s_relay_beacon_timer,
                                             ESPNOW_BEACON_INTERVAL_MS * 1000LL));
    ESP_LOGI(TAG, "Relay beacon started (advertising upstream path)");
}

/* Stop relay beacon broadcasting when the gateway is no longer reachable.
 * Prevents sensors from routing through this node while it has no upstream path. */
static void stop_relay_beacon(void)
{
    if (!s_relay_beacon_started) return;
    s_relay_beacon_started = false;
    if (s_relay_beacon_timer != NULL) {
        esp_timer_stop(s_relay_beacon_timer); /* ignore error if already inactive */
    }
    ESP_LOGW(TAG, "Relay beacon stopped — no upstream path");
}

/* ---------------------------------------------------------------------------
 * ESP-NOW receive callback (relay path)
 * ------------------------------------------------------------------------- */

static void sensor_relay_recv_cb(const esp_now_recv_info_t *info,
                                  const uint8_t *data, int data_len)
{
    if (data_len == (int)sizeof(espnow_beacon_t)) {
        const espnow_beacon_t *b = (const espnow_beacon_t *)data;
        if (b->node_type != ESPNOW_NODE_GATEWAY) {
            return; /* Ignore beacons from other relays. */
        }
        /* Beacon from gateway — cache MAC on first/re-discovery. */
        if (!s_next_hop_valid) {
            bool was_lost = (s_relay_beacon_timer != NULL); /* timer exists = previously seen */
            memcpy(s_next_hop_mac, info->src_addr, 6);
            s_next_hop_valid = true;
            mesh_espnow_add_peer(s_next_hop_mac); /* no-op if already registered */
            if (s_discovery_sem != NULL) {
                xSemaphoreGive(s_discovery_sem);
            }
            ESP_LOGI(TAG, "Gateway %s: " MACSTR,
                     was_lost ? "rediscovered" : "discovered",
                     MAC2STR(s_next_hop_mac));
        }
        /* Start (or restart) relay beacon now that upstream path is valid. */
        start_relay_beacon();
        return;
    }

    if (data_len != (int)sizeof(espnow_payload_t)) {
        return;
    }

    if (!s_next_hop_valid) {
        return; /* Cannot forward without a known gateway. */
    }

    const espnow_payload_t *payload = (const espnow_payload_t *)data;

    /* Dedup check. */
    for (int i = 0; i < ESPNOW_DEDUP_TABLE_SIZE; i++) {
        if (s_dedup_table[i].node_id   == payload->node_id &&
            s_dedup_table[i].msg_count == payload->msg_count) {
            return; /* Already forwarded — drop silently. */
        }
    }
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
 * Scan timer callback — fires in esp_timer task context.
 * Gives semaphore to trigger scan+send in app_main context.
 * ------------------------------------------------------------------------- */

static void scan_timer_cb(void *arg)
{
    BaseType_t woken = pdFALSE;
    xSemaphoreGiveFromISR(s_scan_sem, &woken);
    portYIELD_FROM_ISR(woken);
}

/* ---------------------------------------------------------------------------
 * WiFi RSSI scan (identical pattern to sensor.c)
 * ------------------------------------------------------------------------- */

static void sensor_relay_scan_rssi(int8_t *rssi_out)
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
        if (target_ssids[i][0] != '\0') { any_configured = true; break; }
    }
    if (!any_configured) {
        ESP_LOGW(TAG, "No target SSIDs configured — all RSSI slots will be INT8_MIN");
        return;
    }

    wifi_scan_config_t scan_cfg = {
        .channel     = 0,
        .show_hidden = false,
        .scan_type   = WIFI_SCAN_TYPE_ACTIVE,
        .scan_time   = { .active = { .min = 50, .max = 100 } },
    };

    esp_err_t err = esp_wifi_scan_start(&scan_cfg, true);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "WiFi scan start failed: %s", esp_err_to_name(err));
        return;
    }

    uint16_t ap_count = 0;
    esp_wifi_scan_get_ap_num(&ap_count);
    if (ap_count == 0) {
        ESP_LOGW(TAG, "WiFi scan: no APs found");
        esp_wifi_scan_stop();
        return;
    }

    uint16_t max_records = (ap_count > 20) ? 20 : ap_count;
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&max_records, s_scan_records));
    esp_wifi_scan_stop();

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
 * Scan + send cycle (runs in app_main context)
 * ------------------------------------------------------------------------- */

static void run_scan_cycle(uint8_t node_id)
{
    s_msg_count++;
    ESP_LOGI(TAG, "========== SCAN #%u | node 0x%02X ==========",
             s_msg_count, node_id);

    if (!s_next_hop_valid) {
        ESP_LOGW(TAG, "No gateway known — skipping send, continuing relay");
        return;
    }

    /* Pause relay: unregister recv_cb and deinit ESP-NOW before scanning. */
    esp_now_unregister_recv_cb();
    mesh_espnow_deinit();

    int8_t rssi[3];
    sensor_relay_scan_rssi(rssi);

    /* Restore ESP-NOW on fixed channel. */
    mesh_espnow_set_channel(CONFIG_ESPNOW_CHANNEL);
    mesh_espnow_init();
    mesh_espnow_add_peer(s_next_hop_mac);
    mesh_espnow_add_peer(BROADCAST_MAC);

    s_send_sem = xSemaphoreCreateBinary();
    ESP_ERROR_CHECK(esp_now_register_send_cb(sr_send_cb));
    ESP_ERROR_CHECK(esp_now_register_recv_cb(sensor_relay_recv_cb));

    espnow_payload_t payload = {
        .node_id   = node_id,
        .msg_count = s_msg_count,
        .hop_count = 0,
        .rssi      = { rssi[0], rssi[1], rssi[2] },
        .role      = ESPNOW_ROLE_SENSOR_RELAY,
    };

    esp_err_t err = esp_now_send(s_next_hop_mac, (uint8_t *)&payload, sizeof(payload));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_now_send error: %s", esp_err_to_name(err));
        s_next_hop_valid = false;
        memset(s_next_hop_mac, 0, 6);
        stop_relay_beacon();
        mesh_led_fast_blink();
    } else {
        bool cb_ok = (xSemaphoreTake(s_send_sem, pdMS_TO_TICKS(ESPNOW_SEND_TIMEOUT_MS)) == pdTRUE);
        if (cb_ok && s_send_status == ESP_NOW_SEND_SUCCESS) {
            ESP_LOGI(TAG, "Sending to gateway... OK");
            mesh_led_blink_once();
        } else {
            const char *reason = cb_ok ? "ACK failed" : "send callback timeout";
            ESP_LOGW(TAG, "Gateway unreachable (%s) — will rediscover on next beacon", reason);
            s_next_hop_valid = false;
            memset(s_next_hop_mac, 0, 6);
            stop_relay_beacon();
            mesh_led_fast_blink();
        }
    }

    esp_now_unregister_send_cb();
    vSemaphoreDelete(s_send_sem);
    s_send_sem = NULL;

    ESP_LOGI(TAG, "========== Resuming relay ==========");
}

/* ---------------------------------------------------------------------------
 * Entry point
 * ------------------------------------------------------------------------- */

void sensor_relay_run(void)
{
    mesh_wifi_init();
    mesh_led_init();

    uint8_t node_id = mesh_node_id_get();
    uint8_t mac[6];
    mesh_node_mac_get(mac);
    ESP_LOGI(TAG, "node=0x%02X  mac=" MACSTR "  role=SENSOR_RELAY", node_id, MAC2STR(mac));

    /* Configure light sleep via ESP-IDF PM. The tickless idle hook in FreeRTOS
     * will enter light sleep automatically during vTaskDelay / semaphore waits.
     * The radio stays alive in DTIM listen mode — ESP-NOW recv_cb fires normally. */
    esp_pm_config_t pm_cfg = {
        .max_freq_mhz      = CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ,
        .min_freq_mhz      = 10,
        .light_sleep_enable = true,
    };
    esp_err_t pm_err = esp_pm_configure(&pm_cfg);
    if (pm_err != ESP_OK) {
        ESP_LOGW(TAG, "esp_pm_configure failed (%s) — running without light sleep",
                 esp_err_to_name(pm_err));
    } else {
        ESP_LOGI(TAG, "Light sleep enabled (max=%d MHz, min=10 MHz)",
                 CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ);
    }

    /* Gateway discovery. */
    mesh_espnow_set_channel(CONFIG_ESPNOW_CHANNEL);
    mesh_espnow_init();

    s_discovery_sem = xSemaphoreCreateBinary();
    ESP_ERROR_CHECK(esp_now_register_recv_cb(sensor_relay_recv_cb));

    bool found = (xSemaphoreTake(s_discovery_sem,
                                 pdMS_TO_TICKS(ESPNOW_DISCOVERY_TIMEOUT_MS)) == pdTRUE);
    vSemaphoreDelete(s_discovery_sem);
    s_discovery_sem = NULL;

    if (found) {
        /* Gateway found — start advertising ourselves so sensors can route via us. */
        start_relay_beacon();
    } else {
        /* Continue in receive mode — recv_cb will start beacon on next gateway beacon. */
        ESP_LOGW(TAG, "No gateway found within %d ms — continuing in relay-only mode",
                 ESPNOW_DISCOVERY_TIMEOUT_MS);
        /* Broadcast peer still needed to receive future gateway beacons. */
        mesh_espnow_add_peer(BROADCAST_MAC);
    }

    /* Periodic scan timer. */
    s_scan_sem = xSemaphoreCreateBinary();
    esp_timer_handle_t scan_timer;
    const esp_timer_create_args_t timer_args = {
        .callback        = scan_timer_cb,
        .arg             = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name            = "sr_scan",
    };
    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &scan_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(scan_timer,
                    (uint64_t)CONFIG_SENSOR_RELAY_SCAN_INTERVAL_SEC * 1000000ULL));

    ESP_ERROR_CHECK(esp_task_wdt_add(NULL));

    xTaskCreate(mesh_led_task, "mesh_led", CONFIG_MESH_LED_TASK_STACK_SIZE,
                NULL, 2, &s_led_task_handle);

    ESP_LOGI(TAG, "Relay active — scan every %d s", CONFIG_SENSOR_RELAY_SCAN_INTERVAL_SEC);

    /* Steady-state: light sleep between events.
     * Semaphore take with 5 s timeout feeds the WDT while waiting. */
    while (1) {
        if (xSemaphoreTake(s_scan_sem, pdMS_TO_TICKS(5000)) == pdTRUE) {
            run_scan_cycle(node_id);
        }
        esp_task_wdt_reset();
    }
}
