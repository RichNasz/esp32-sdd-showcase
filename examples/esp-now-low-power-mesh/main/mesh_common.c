/* ================================================
   AGENT-GENERATED — DO NOT EDIT BY HAND
   Generated from specs/ using esp32-sdd-full-project-generator skill
   Date: 2026-04-08 | Agent: Claude Code
   ================================================ */

#include "mesh_common.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_now.h"
#include "esp_mac.h"
#include "driver/gpio.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

#if CONFIG_MESH_LED_WS2812
#include "driver/rmt_tx.h"
#include "driver/rmt_encoder.h"
#endif

static const char *TAG = "mesh";

/* ---------------------------------------------------------------------------
 * WS2812B RMT driver (compiled only when MESH_LED_WS2812=y)
 *
 * WS2812B uses GRB byte order and NRZ timing at ~800 kHz.
 * RMT resolution: 10 MHz → 1 tick = 100 ns.
 *   Bit-0: T0H = 3 ticks (300 ns), T0L = 9 ticks (900 ns)
 *   Bit-1: T1H = 9 ticks (900 ns), T1L = 3 ticks (300 ns)
 * Both values are within the WS2812B ±150 ns tolerance band.
 * ------------------------------------------------------------------------- */

#if CONFIG_MESH_LED_WS2812

#define WS2812_RMT_RESOLUTION_HZ  (10 * 1000 * 1000)  /* 10 MHz */
#define WS2812_T0H_TICKS  3   /* 300 ns high for a 0-bit */
#define WS2812_T0L_TICKS  9   /* 900 ns low  for a 0-bit */
#define WS2812_T1H_TICKS  9   /* 900 ns high for a 1-bit */
#define WS2812_T1L_TICKS  3   /* 300 ns low  for a 1-bit */

static rmt_channel_handle_t s_rmt_chan;
static rmt_encoder_handle_t s_bytes_enc;
static const rmt_transmit_config_t s_tx_cfg = {
    .loop_count    = 0,
    .flags.eot_level = 0,  /* hold line low after frame — WS2812 reset */
};

static void ws2812_write(uint8_t r, uint8_t g, uint8_t b)
{
    uint8_t grb[3] = {g, r, b};  /* WS2812B expects GRB order */
    rmt_transmit(s_rmt_chan, s_bytes_enc, grb, sizeof(grb), &s_tx_cfg);
    rmt_tx_wait_all_done(s_rmt_chan, pdMS_TO_TICKS(100));
}

/* Dim colour values to avoid eye-blasting brightness on a bare LED. */
#define WS2812_GREEN  0, 24,  0   /* success / activity */
#define WS2812_RED   24,  0,  0   /* failure             */
#define WS2812_OFF    0,  0,  0   /* idle                */

static void led_set(bool on, bool error)
{
    if (!on) {
        ws2812_write(WS2812_OFF);
    } else if (error) {
        ws2812_write(WS2812_RED);
    } else {
        ws2812_write(WS2812_GREEN);
    }
}

#else  /* simple GPIO LED */

/* When active-LOW (Kconfig y): GPIO low = ON. When active-HIGH (Kconfig n): GPIO high = ON.
 * Use #ifdef because a Kconfig bool set to 'n' is not defined at all in sdkconfig.h. */
#ifdef CONFIG_MESH_BLINK_ACTIVE_LOW
#define LED_ON_LEVEL  0
#define LED_OFF_LEVEL 1
#else
#define LED_ON_LEVEL  1
#define LED_OFF_LEVEL 0
#endif

static void led_set(bool on, bool error)
{
    (void)error;  /* simple GPIO has no colour — on/off only */
    gpio_set_level(CONFIG_MESH_BLINK_GPIO, on ? LED_ON_LEVEL : LED_OFF_LEVEL);
}

#endif  /* CONFIG_MESH_LED_WS2812 */

/* ---------------------------------------------------------------------------
 * WiFi lifecycle
 * ------------------------------------------------------------------------- */

void mesh_wifi_init(void)
{
    /* NVS must be initialised before WiFi (PHY calibration data storage). */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
}

void mesh_wifi_deinit(void)
{
    ESP_ERROR_CHECK(esp_wifi_stop());
    ESP_ERROR_CHECK(esp_wifi_deinit());
}

/* ---------------------------------------------------------------------------
 * ESP-NOW lifecycle
 * ------------------------------------------------------------------------- */

void mesh_espnow_init(void)
{
    ESP_ERROR_CHECK(esp_now_init());
}

void mesh_espnow_deinit(void)
{
    ESP_ERROR_CHECK(esp_now_deinit());
}

void mesh_espnow_set_channel(uint8_t channel)
{
    ESP_ERROR_CHECK(esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE));
}

void mesh_espnow_add_peer(const uint8_t *mac)
{
    esp_now_peer_info_t peer = {
        .channel = CONFIG_ESPNOW_CHANNEL,
        .ifidx   = WIFI_IF_STA,
        .encrypt = false,
    };
    memcpy(peer.peer_addr, mac, 6);

    esp_err_t err = esp_now_add_peer(&peer);
    if (err != ESP_OK && err != ESP_ERR_ESPNOW_EXIST) {
        ESP_LOGE(TAG, "esp_now_add_peer failed: %s", esp_err_to_name(err));
    }
}

/* ---------------------------------------------------------------------------
 * Node identity
 * ------------------------------------------------------------------------- */

uint8_t mesh_node_id_get(void)
{
    uint8_t mac[6];
    ESP_ERROR_CHECK(esp_read_mac(mac, ESP_MAC_WIFI_STA));
    /* Fall back to mac[4] if mac[5] is a pathological value. */
    uint8_t id = mac[5];
    if (id == 0x00 || id == 0xFF) {
        id = mac[4];
    }
    return id;
}

void mesh_node_mac_get(uint8_t *mac_out)
{
    ESP_ERROR_CHECK(esp_read_mac(mac_out, ESP_MAC_WIFI_STA));
}

/* ---------------------------------------------------------------------------
 * LED helpers
 * ------------------------------------------------------------------------- */

void mesh_led_init(void)
{
#if CONFIG_MESH_LED_WS2812
    rmt_tx_channel_config_t tx_cfg = {
        .gpio_num          = CONFIG_MESH_BLINK_GPIO,
        .clk_src           = RMT_CLK_SRC_DEFAULT,
        .resolution_hz     = WS2812_RMT_RESOLUTION_HZ,
        .mem_block_symbols = 64,
        .trans_queue_depth = 4,
    };
    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_cfg, &s_rmt_chan));

    rmt_bytes_encoder_config_t enc_cfg = {
        .bit0 = {
            .level0    = 1, .duration0 = WS2812_T0H_TICKS,
            .level1    = 0, .duration1 = WS2812_T0L_TICKS,
        },
        .bit1 = {
            .level0    = 1, .duration0 = WS2812_T1H_TICKS,
            .level1    = 0, .duration1 = WS2812_T1L_TICKS,
        },
        .flags.msb_first = 1,
    };
    ESP_ERROR_CHECK(rmt_new_bytes_encoder(&enc_cfg, &s_bytes_enc));
    ESP_ERROR_CHECK(rmt_enable(s_rmt_chan));

    ws2812_write(WS2812_OFF);  /* ensure LED starts off */
#else
    /* Disable any pad hold left from a previous deep sleep cycle. */
    gpio_hold_dis(CONFIG_MESH_BLINK_GPIO);

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << CONFIG_MESH_BLINK_GPIO),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));
    led_set(false, false);
#endif
}

void mesh_led_on(void)
{
    led_set(true, false);
}

void mesh_led_off(void)
{
    led_set(false, false);
}

void mesh_led_blink_once(void)
{
    /* Green on WS2812, on-level on simple GPIO — signals success/activity. */
    led_set(true, false);
    vTaskDelay(pdMS_TO_TICKS(20));
    led_set(false, false);
}

void mesh_led_fast_blink(void)
{
    /* Red on WS2812, toggled on simple GPIO — signals send failure. */
    for (int i = 0; i < 3; i++) {
        led_set(true, true);
        vTaskDelay(pdMS_TO_TICKS(100));
        led_set(false, false);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void mesh_led_task(void *arg)
{
    /* Priority 2 ensures blink response is immediate regardless of app_main load.
     * pdFALSE decrements (not clears) the notification count so rapid-fire
     * notifications each produce a separate blink. */
    while (1) {
        ulTaskNotifyTake(pdFALSE, portMAX_DELAY);
        led_set(true, false);
        vTaskDelay(pdMS_TO_TICKS(20));
        led_set(false, false);
    }
}
