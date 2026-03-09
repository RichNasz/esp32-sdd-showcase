/* AGENT-GENERATED — do not edit by hand; regenerate via esp32-sdd-full-project-generator */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include "esp_log.h"

static const char *TAG = "blinky";

#define LED_GPIO   CONFIG_EXAMPLE_LED_GPIO
#define LED_ON     CONFIG_EXAMPLE_LED_ACTIVE_LEVEL
#define LED_OFF    (1 - CONFIG_EXAMPLE_LED_ACTIVE_LEVEL)

#ifdef CONFIG_IDF_TARGET_ESP32
#define LEDC_MODE LEDC_HIGH_SPEED_MODE
#else
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
        /* Note: ESP_ERROR_CHECK must NOT be used here — esp_log/abort are
           unsafe in ISR/IRAM context. Calls are intentionally bare. */
        ledc_set_fade_with_time(LEDC_MODE, LEDC_CHANNEL, next, FADE_TIME_MS);
        ledc_fade_start(LEDC_MODE, LEDC_CHANNEL, LEDC_FADE_NO_WAIT);
        ESP_DRAM_LOGI(TAG, "Fade %s → duty %lu",
                      fading_to_on ? "up" : "down", (unsigned long)next);
    }
    return higher_prio_woken == pdTRUE;
}

void app_main(void)
{
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
}
