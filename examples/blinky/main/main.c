/* ================================================
   AGENT-GENERATED — DO NOT EDIT BY HAND
   Generated from specs/ using esp32-sdd-full-project-generator skill
   Date: 2026-04-12 | Agent: Claude Code (updated 2026-04-12)
   ================================================ */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "sdkconfig.h"

#if CONFIG_EXAMPLE_LED_WS2812
#include "driver/rmt_tx.h"
#include "driver/rmt_encoder.h"
#else
#include "driver/ledc.h"
#endif

static const char *TAG = "blinky";

#define LED_GPIO CONFIG_EXAMPLE_LED_GPIO

/* ============================================================
 * WS2812 path — compiled only when EXAMPLE_LED_WS2812=y
 *
 * WS2812B uses GRB byte order and NRZ timing at ~800 kHz.
 * RMT resolution: 10 MHz → 1 tick = 100 ns.
 *   Bit-0: T0H = 3 ticks (300 ns), T0L = 9 ticks (900 ns)
 *   Bit-1: T1H = 9 ticks (900 ns), T1L = 3 ticks (300 ns)
 * Both values are within the WS2812B ±150 ns tolerance band.
 * ============================================================ */
#if CONFIG_EXAMPLE_LED_WS2812

#define WS2812_RMT_RESOLUTION_HZ  (10 * 1000 * 1000)  /* 10 MHz */
#define WS2812_T0H_TICKS  3   /* 300 ns high for a 0-bit */
#define WS2812_T0L_TICKS  9   /* 900 ns low  for a 0-bit */
#define WS2812_T1H_TICKS  9   /* 900 ns high for a 1-bit */
#define WS2812_T1L_TICKS  3   /* 300 ns low  for a 1-bit */

/* Color table — each entry at max brightness (64/255) to protect eyes on bare LEDs.
 * The active color advances by one entry at the start of each new breath cycle. */
static const struct { uint8_t r, g, b; } s_colors[] = {
    {64,  0,  0},  /* red     */
    { 0, 64,  0},  /* green   */
    { 0,  0, 64},  /* blue    */
    { 0, 64, 64},  /* cyan    */
    {64,  0, 64},  /* magenta */
    {64, 32,  0},  /* amber   */
};
#define WS2812_NUM_COLORS  (sizeof(s_colors) / sizeof(s_colors[0]))
static int s_color_idx = 0;

#define WS2812_FADE_STEPS  100          /* steps per ramp                       */
#define WS2812_STEP_MS     20           /* 100 × 20 ms = 2 s per ramp           */

static rmt_channel_handle_t s_rmt_chan;
static rmt_encoder_handle_t s_bytes_enc;
static const rmt_transmit_config_t s_tx_cfg = {
    .loop_count      = 0,
    .flags.eot_level = 0,  /* hold line LOW after frame — WS2812 reset condition */
};

static void ws2812_write(uint8_t r, uint8_t g, uint8_t b)
{
    uint8_t grb[3] = {g, r, b};  /* WS2812B expects GRB order */
    rmt_transmit(s_rmt_chan, s_bytes_enc, grb, sizeof(grb), &s_tx_cfg);
    rmt_tx_wait_all_done(s_rmt_chan, pdMS_TO_TICKS(100));
}

static void ws2812_init(void)
{
    rmt_tx_channel_config_t tx_cfg = {
        .gpio_num          = LED_GPIO,
        .clk_src           = RMT_CLK_SRC_DEFAULT,
        .resolution_hz     = WS2812_RMT_RESOLUTION_HZ,
        .mem_block_symbols = 64,
        .trans_queue_depth = 4,
    };
    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_cfg, &s_rmt_chan));

    rmt_bytes_encoder_config_t enc_cfg = {
        .bit0 = {
            .level0 = 1, .duration0 = WS2812_T0H_TICKS,
            .level1 = 0, .duration1 = WS2812_T0L_TICKS,
        },
        .bit1 = {
            .level0 = 1, .duration0 = WS2812_T1H_TICKS,
            .level1 = 0, .duration1 = WS2812_T1L_TICKS,
        },
        .flags.msb_first = 1,
    };
    ESP_ERROR_CHECK(rmt_new_bytes_encoder(&enc_cfg, &s_bytes_enc));
    ESP_ERROR_CHECK(rmt_enable(s_rmt_chan));

    ws2812_write(0, 0, 0);  /* ensure LED starts off */
    ESP_LOGI(TAG, "WS2812 RMT channel init on GPIO %d (10 MHz, GRB order, max brightness 64/255)",
             LED_GPIO);
}

/* ============================================================
 * LEDC path — compiled only when EXAMPLE_LED_WS2812=n
 * ============================================================ */
#else

#define LED_ON   CONFIG_EXAMPLE_LED_ACTIVE_LEVEL
#define LED_OFF  (1 - CONFIG_EXAMPLE_LED_ACTIVE_LEVEL)

#ifdef CONFIG_IDF_TARGET_ESP32
/* Original ESP32: dual LEDC subsystem — high-speed timers are available. */
#define LEDC_MODE LEDC_HIGH_SPEED_MODE
#else
/* All other targets (S3, C5, C6, …): only low-speed mode exists. */
#define LEDC_MODE LEDC_LOW_SPEED_MODE
#endif

#define LEDC_TIMER      LEDC_TIMER_0
#define LEDC_CHANNEL    LEDC_CHANNEL_0
#define LEDC_FREQ_HZ    5000
#define LEDC_RESOLUTION LEDC_TIMER_13_BIT
#define FADE_TIME_MS    2000

#define DUTY_MAX  ((1 << LEDC_RESOLUTION) - 1)
#define DUTY_ON   (LED_ON  ? DUTY_MAX : 0)
#define DUTY_OFF  (LED_OFF ? DUTY_MAX : 0)

/* true  = currently fading toward DUTY_ON  (brightening)
   false = currently fading toward DUTY_OFF (dimming)   */
static volatile bool fading_to_on = true;

static bool IRAM_ATTR ledc_fade_done_cb(const ledc_cb_param_t *param, void *arg)
{
    BaseType_t higher_prio_woken = pdFALSE;
    if (param->event == LEDC_FADE_END_EVT) {
        fading_to_on = !fading_to_on;
        uint32_t next = fading_to_on ? DUTY_ON : DUTY_OFF;
        /* ESP_ERROR_CHECK must NOT be used here — abort() is unsafe from ISR context. */
        ledc_set_fade_with_time(LEDC_MODE, LEDC_CHANNEL, next, FADE_TIME_MS);
        ledc_fade_start(LEDC_MODE, LEDC_CHANNEL, LEDC_FADE_NO_WAIT);
        ESP_DRAM_LOGI(TAG, "Fade %s → duty %lu",
                      fading_to_on ? "up" : "down", (unsigned long)next);
    }
    return higher_prio_woken == pdTRUE;
}

#endif  /* CONFIG_EXAMPLE_LED_WS2812 */

/* ============================================================
 * Entry point
 * ============================================================ */
void app_main(void)
{
#if CONFIG_EXAMPLE_LED_WS2812

    ESP_LOGI(TAG, "Blinky — WS2812 RGB LED, GPIO %d, breathing period %d ms",
             LED_GPIO, WS2812_FADE_STEPS * WS2812_STEP_MS * 2);
    ws2812_init();
    ESP_LOGI(TAG, "Starting WS2812 breathing loop");

    while (1) {
        uint8_t r = s_colors[s_color_idx].r;
        uint8_t g = s_colors[s_color_idx].g;
        uint8_t b = s_colors[s_color_idx].b;
        ESP_LOGI(TAG, "Cycle color %d/%d  r=%d g=%d b=%d",
                 s_color_idx + 1, (int)WS2812_NUM_COLORS, r, g, b);

        /* Ramp up: dark -> color at full brightness */
        for (int i = 0; i <= WS2812_FADE_STEPS; i++) {
            ws2812_write((uint8_t)((r * i) / WS2812_FADE_STEPS),
                         (uint8_t)((g * i) / WS2812_FADE_STEPS),
                         (uint8_t)((b * i) / WS2812_FADE_STEPS));
            vTaskDelay(pdMS_TO_TICKS(WS2812_STEP_MS));
        }
        /* Ramp down: color at full brightness -> dark */
        for (int i = WS2812_FADE_STEPS; i >= 0; i--) {
            ws2812_write((uint8_t)((r * i) / WS2812_FADE_STEPS),
                         (uint8_t)((g * i) / WS2812_FADE_STEPS),
                         (uint8_t)((b * i) / WS2812_FADE_STEPS));
            vTaskDelay(pdMS_TO_TICKS(WS2812_STEP_MS));
        }

        s_color_idx = (s_color_idx + 1) % (int)WS2812_NUM_COLORS;
    }

#else

    ESP_LOGI(TAG, "Board GPIO=%d  freq=%d Hz  resolution=%d-bit  polarity=active-%s",
             LED_GPIO, LEDC_FREQ_HZ, LEDC_RESOLUTION,
             LED_ON ? "HIGH" : "LOW");
    ESP_LOGI(TAG, "Breathing period: %d ms", FADE_TIME_MS * 2);

    ledc_timer_config_t timer = {
        .speed_mode      = LEDC_MODE,
        .duty_resolution = LEDC_RESOLUTION,
        .timer_num       = LEDC_TIMER,
        .freq_hz         = LEDC_FREQ_HZ,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer));

    ledc_channel_config_t channel = {
        .speed_mode = LEDC_MODE,
        .channel    = LEDC_CHANNEL,
        .timer_sel  = LEDC_TIMER,
        .intr_type  = LEDC_INTR_DISABLE,
        .gpio_num   = LED_GPIO,
        .duty       = DUTY_OFF,   /* start dark regardless of polarity */
        .hpoint     = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&channel));

    ESP_ERROR_CHECK(ledc_fade_func_install(0));

    ledc_cbs_t cbs = { .fade_cb = ledc_fade_done_cb };
    ESP_ERROR_CHECK(ledc_cb_register(LEDC_MODE, LEDC_CHANNEL, &cbs, NULL));

    /* First fade: dark → bright (DUTY_OFF → DUTY_ON) */
    ESP_LOGI(TAG, "Fade up → duty %lu", (unsigned long)DUTY_ON);
    ESP_ERROR_CHECK(ledc_set_fade_with_time(LEDC_MODE, LEDC_CHANNEL, DUTY_ON, FADE_TIME_MS));
    ESP_ERROR_CHECK(ledc_fade_start(LEDC_MODE, LEDC_CHANNEL, LEDC_FADE_NO_WAIT));

    /* All subsequent breathing is driven by ledc_fade_done_cb */
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

#endif  /* CONFIG_EXAMPLE_LED_WS2812 */
}
