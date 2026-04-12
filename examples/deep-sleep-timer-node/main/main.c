/* ================================================
   AGENT-GENERATED — DO NOT EDIT BY HAND
   Generated from specs/ using esp32-sdd-full-project-generator skill
   Date: 2026-04-12 | Agent: Claude Code
   ================================================ */

#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_random.h"

#if CONFIG_EXAMPLE_LED_WS2812
#include "driver/rmt_tx.h"
#include "driver/rmt_encoder.h"
#endif

static const char *TAG = "sleep-node";

/* Survives deep sleep; resets only on power-cycle or power-on reset. */
static RTC_DATA_ATTR uint32_t boot_count;

#define LED_GPIO           CONFIG_EXAMPLE_LED_GPIO
#define SLEEP_DURATION_US  (5ULL * 1000 * 1000)   /* 5 s in microseconds */
#define BLINK_ON_MS        100
#define BLINK_OFF_MS       100

/* ================================================================
   WS2812 RMT path — compiled when CONFIG_EXAMPLE_LED_WS2812=y
   Generated per esp32-ws2812-led-engineer skill workflow.
   ================================================================ */
#if CONFIG_EXAMPLE_LED_WS2812

#define WS2812_RMT_RESOLUTION_HZ  (10 * 1000 * 1000)  /* 10 MHz → 1 tick = 100 ns */
#define WS2812_T0H_TICKS  3   /* 300 ns high for a 0-bit */
#define WS2812_T0L_TICKS  9   /* 900 ns low  for a 0-bit */
#define WS2812_T1H_TICKS  9   /* 900 ns high for a 1-bit */
#define WS2812_T1L_TICKS  3   /* 300 ns low  for a 1-bit */

static rmt_channel_handle_t s_rmt_chan;
static rmt_encoder_handle_t s_bytes_enc;
static const rmt_transmit_config_t s_tx_cfg = {
    .loop_count      = 0,
    .flags.eot_level = 0,  /* hold line LOW after frame — WS2812 reset condition */
};

static void ws2812_write(uint8_t r, uint8_t g, uint8_t b)
{
    uint8_t grb[3] = {g, r, b};  /* WS2812B expects GRB byte order, not RGB */
    rmt_transmit(s_rmt_chan, s_bytes_enc, grb, sizeof(grb), &s_tx_cfg);
    rmt_tx_wait_all_done(s_rmt_chan, pdMS_TO_TICKS(100));
}

typedef struct { uint8_t r, g, b; } rgb_t;

/* Standard 6-colour palette — every entry has at least one channel at 64/255.
   8/255 is invisible under normal room lighting; 64/255 is the enforced floor
   (see shared-specs/AIGenLessonsLearned.md). */
static const rgb_t s_palette[] = {
    {64,  0,  0},   /* red     */
    { 0, 64,  0},   /* green   */
    { 0,  0, 64},   /* blue    */
    { 0, 64, 64},   /* cyan    */
    {64,  0, 64},   /* magenta */
    {64, 32,  0},   /* amber   */
};

#define PALETTE_LEN  (sizeof(s_palette) / sizeof(s_palette[0]))

_Static_assert(PALETTE_LEN == 6, "palette must have exactly 6 entries");

static void led_init(void)
{
    gpio_hold_dis(LED_GPIO);

    rmt_tx_channel_config_t tx_cfg = {
        .gpio_num          = LED_GPIO,
        .clk_src           = RMT_CLK_SRC_DEFAULT,
        .resolution_hz     = WS2812_RMT_RESOLUTION_HZ,
        .mem_block_symbols = 64,
        .trans_queue_depth = 4,
    };
    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_cfg, &s_rmt_chan));

    rmt_bytes_encoder_config_t enc_cfg = {
        .bit0 = { .level0 = 1, .duration0 = WS2812_T0H_TICKS,
                  .level1 = 0, .duration1 = WS2812_T0L_TICKS },
        .bit1 = { .level0 = 1, .duration0 = WS2812_T1H_TICKS,
                  .level1 = 0, .duration1 = WS2812_T1L_TICKS },
        .flags.msb_first = 1,
    };
    ESP_ERROR_CHECK(rmt_new_bytes_encoder(&enc_cfg, &s_bytes_enc));
    ESP_ERROR_CHECK(rmt_enable(s_rmt_chan));

    ws2812_write(0, 0, 0);  /* ensure LED starts dark regardless of prior state */
    ESP_LOGI(TAG, "WS2812 RMT init on GPIO %d (10 MHz, GRB order)", LED_GPIO);
}

static void led_heartbeat(void)
{
    /* Select a random colour from the palette using the hardware entropy source.
       esp_random() requires no seeding and is always available on ESP32 targets. */
    uint32_t idx = esp_random() % PALETTE_LEN;
    const rgb_t *c = &s_palette[idx];
    ws2812_write(c->r, c->g, c->b);
    vTaskDelay(pdMS_TO_TICKS(BLINK_ON_MS));
    ws2812_write(0, 0, 0);
    vTaskDelay(pdMS_TO_TICKS(BLINK_OFF_MS));
}

/* ================================================================
   Simple GPIO path — compiled when CONFIG_EXAMPLE_LED_WS2812=n
   No PWM or LEDC required — this is a single brief on/off blink.
   ================================================================ */
#else

static void led_init(void)
{
    /* Release any GPIO hold set by the previous deep-sleep cycle before
       driving the pin. Without this, gpio_set_level() has no effect. */
    gpio_hold_dis(LED_GPIO);

    const gpio_config_t io_cfg = {
        .pin_bit_mask = (1ULL << LED_GPIO),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&io_cfg));
    gpio_set_level(LED_GPIO, 1 - CONFIG_EXAMPLE_LED_ACTIVE_LEVEL);  /* inactive = LED off */
}

static void led_heartbeat(void)
{
    gpio_set_level(LED_GPIO, CONFIG_EXAMPLE_LED_ACTIVE_LEVEL);           /* LED on  */
    vTaskDelay(pdMS_TO_TICKS(BLINK_ON_MS));
    gpio_set_level(LED_GPIO, 1 - CONFIG_EXAMPLE_LED_ACTIVE_LEVEL);      /* LED off */
    vTaskDelay(pdMS_TO_TICKS(BLINK_OFF_MS));
}

#endif  /* CONFIG_EXAMPLE_LED_WS2812 */

/* ================================================================
   Main — executes once per wake cycle, then enters deep sleep.
   Generated per esp32-deep-sleep-engineer skill workflow.
   ================================================================ */
void app_main(void)
{
    /* Increment counter as the very first action — before any conditional
       branching — so every wakeup cycle produces an accurate count. */
    boot_count++;

    /* USB-CDC boards (e.g. XIAO ESP32S3 configured as a secondary board)
       need ~50 ms for the USB device to enumerate before the first log line
       is visible. UART bridge and USB Serial/JTAG boards need no delay. */
#if CONFIG_ESP_CONSOLE_USB_CDC
    vTaskDelay(pdMS_TO_TICKS(50));
#endif

    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    if (cause == ESP_SLEEP_WAKEUP_UNDEFINED) {
        ESP_LOGI(TAG, "First boot — counter resets to 0 on power-cycle");
    } else if (cause == ESP_SLEEP_WAKEUP_TIMER) {
        ESP_LOGI(TAG, "Wake #%" PRIu32 " — 5 s timer wakeup", boot_count);
    } else {
        /* Unrecognised causes must not abort — log and continue to sleep. */
        ESP_LOGW(TAG, "Wake #%" PRIu32 " — unexpected wakeup cause %d; proceeding to sleep",
                 boot_count, (int)cause);
    }

    led_init();
    led_heartbeat();

    ESP_LOGI(TAG, "Sleeping for 5 s");
    esp_sleep_enable_timer_wakeup(SLEEP_DURATION_US);
    esp_deep_sleep_start();
}
