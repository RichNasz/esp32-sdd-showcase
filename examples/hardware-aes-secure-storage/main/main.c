// ================================================
// AGENT-GENERATED — DO NOT EDIT BY HAND
// Generated from specs/ using esp32-sdd-full-project-generator skill
// Date: 2026-04-12 | Agent: Claude Code
// ================================================

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"

#if CONFIG_EXAMPLE_LED_WS2812
#include "driver/rmt_tx.h"
#include "driver/rmt_encoder.h"
#else
#include "driver/gpio.h"
#endif

#include "secure_storage.h"

static const char *TAG = "aes_demo";

/* --------------------------------------------------------------------------
 * Constants
 * -------------------------------------------------------------------------- */
#define BENCH_BYTES         (64 * 1024)       /* 64 KB benchmark payload */
#define BLINK_PASS_DELAY_MS 500               /* 1× slow blink on pass   */
#define BLINK_FAIL_DELAY_MS 100               /* 3× fast blink on fail   */
#define SELF_TEST_KEY       "selftest"        /* NVS key; max 15 chars   */
#define SELF_TEST_PLAINTEXT "Hello, AES-256-CBC hardware!"

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

#define LED_GPIO                   CONFIG_EXAMPLE_LED_GPIO
#define WS2812_RMT_RESOLUTION_HZ   (10 * 1000 * 1000)  /* 10 MHz */
#define WS2812_T0H_TICKS  3   /* 300 ns high for a 0-bit */
#define WS2812_T0L_TICKS  9   /* 900 ns low  for a 0-bit */
#define WS2812_T1H_TICKS  9   /* 900 ns high for a 1-bit */
#define WS2812_T1L_TICKS  3   /* 300 ns low  for a 1-bit */

/* Clearly visible colours capped at 64/255 to protect eyes on bare LEDs.
 * Pass = green (slow, calm), Fail = red (fast, urgent). */
#define WS2812_PASS_R  0
#define WS2812_PASS_G  64
#define WS2812_PASS_B  0
#define WS2812_FAIL_R  64
#define WS2812_FAIL_G  0
#define WS2812_FAIL_B  0

static rmt_channel_handle_t s_rmt_chan;
static rmt_encoder_handle_t s_bytes_enc;
static const rmt_transmit_config_t s_tx_cfg = {
    .loop_count      = 0,
    .flags.eot_level = 0,   /* hold line LOW after frame — WS2812 reset condition */
};

static void ws2812_write(uint8_t r, uint8_t g, uint8_t b)
{
    uint8_t grb[3] = {g, r, b};  /* WS2812B expects GRB order */
    rmt_transmit(s_rmt_chan, s_bytes_enc, grb, sizeof(grb), &s_tx_cfg);
    rmt_tx_wait_all_done(s_rmt_chan, pdMS_TO_TICKS(100));
}

static void led_init(void)
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
    ESP_LOGI(TAG, "WS2812 RMT channel init on GPIO %d (10 MHz, GRB, dim status mode)",
             LED_GPIO);
}

static void led_blink_pass(void)
{
    /* Three slow green blinks: 500 ms on, 500 ms off. */
    for (int i = 0; i < 3; i++) {
        ws2812_write(WS2812_PASS_R, WS2812_PASS_G, WS2812_PASS_B);
        vTaskDelay(pdMS_TO_TICKS(BLINK_PASS_DELAY_MS));
        ws2812_write(0, 0, 0);
        vTaskDelay(pdMS_TO_TICKS(BLINK_PASS_DELAY_MS));
    }
}

static void led_blink_fail(void)
{
    /* Three fast blinks in red to signal self-test failure. */
    for (int i = 0; i < 3; i++) {
        ws2812_write(WS2812_FAIL_R, WS2812_FAIL_G, WS2812_FAIL_B);
        vTaskDelay(pdMS_TO_TICKS(BLINK_FAIL_DELAY_MS));
        ws2812_write(0, 0, 0);
        vTaskDelay(pdMS_TO_TICKS(BLINK_FAIL_DELAY_MS));
    }
}

/* ============================================================
 * GPIO path — compiled only when EXAMPLE_LED_WS2812=n
 * ============================================================ */
#else

#define LED_GPIO    CONFIG_EXAMPLE_LED_GPIO
#define LED_ON      CONFIG_EXAMPLE_LED_ACTIVE_LEVEL
#define LED_OFF     (1 - CONFIG_EXAMPLE_LED_ACTIVE_LEVEL)

static void led_init(void)
{
    gpio_config_t cfg = {
        .pin_bit_mask = (1ULL << LED_GPIO),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&cfg));
    gpio_set_level(LED_GPIO, LED_OFF);
}

static void led_blink_pass(void)
{
    /* One slow blink: LED on for 500 ms, off for 500 ms. */
    gpio_set_level(LED_GPIO, LED_ON);
    vTaskDelay(pdMS_TO_TICKS(BLINK_PASS_DELAY_MS));
    gpio_set_level(LED_GPIO, LED_OFF);
    vTaskDelay(pdMS_TO_TICKS(BLINK_PASS_DELAY_MS));
}

static void led_blink_fail(void)
{
    /* Three fast blinks to signal self-test failure. */
    for (int i = 0; i < 3; i++) {
        gpio_set_level(LED_GPIO, LED_ON);
        vTaskDelay(pdMS_TO_TICKS(BLINK_FAIL_DELAY_MS));
        gpio_set_level(LED_GPIO, LED_OFF);
        vTaskDelay(pdMS_TO_TICKS(BLINK_FAIL_DELAY_MS));
    }
}

#endif  /* CONFIG_EXAMPLE_LED_WS2812 */

/* --------------------------------------------------------------------------
 * Self-test
 * -------------------------------------------------------------------------- */

static esp_err_t run_self_test(void)
{
    const char   *plaintext = SELF_TEST_PLAINTEXT;
    const size_t  pt_len    = strlen(plaintext);

    ESP_LOGI(TAG, "Self-test: encrypting '%s'", plaintext);
    esp_err_t err = secure_write(SELF_TEST_KEY, plaintext, pt_len);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Self-test: secure_write failed: %s", esp_err_to_name(err));
        return err;
    }

    char   recovered[64] = {0};
    size_t rlen          = sizeof(recovered) - 1;   /* reserve space for null */

    ESP_LOGI(TAG, "Self-test: reading from NVS and decrypting");
    err = secure_read(SELF_TEST_KEY, recovered, &rlen);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Self-test: secure_read failed: %s", esp_err_to_name(err));
        return err;
    }

    if (rlen != pt_len || memcmp(recovered, plaintext, pt_len) != 0) {
        ESP_LOGE(TAG, "Self-test: FAIL — plaintext mismatch");
        ESP_LOGE(TAG, "  expected (%zu bytes): %s", pt_len, plaintext);
        ESP_LOGE(TAG, "  got      (%zu bytes): %.*s", rlen, (int)rlen, recovered);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Self-test: PASS — plaintext matches");
    return ESP_OK;
}

/* --------------------------------------------------------------------------
 * Throughput benchmark
 * -------------------------------------------------------------------------- */

static void run_benchmark(void)
{
    ESP_LOGI(TAG, "Benchmark: encrypting %u KB with hardware AES-256-CBC",
             BENCH_BYTES / 1024);

    float throughput = 0.0f;
    esp_err_t err = secure_storage_benchmark(BENCH_BYTES, &throughput);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Benchmark failed: %s", esp_err_to_name(err));
        return;
    }

    ESP_LOGI(TAG, "Benchmark: %.2f MB/s", throughput);
    ESP_LOGI(TAG, "  (hardware AES >= 5 MB/s on Xtensa; >= 2 MB/s on RISC-V)");
}

/* --------------------------------------------------------------------------
 * Entry point
 * -------------------------------------------------------------------------- */

void app_main(void)
{
    ESP_LOGI(TAG, "Hardware AES Secure Storage example starting");

    led_init();

    /* NVS initialisation — erase and reinit if partition is full or version changed */
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS partition needs erase, reinitialising");
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    ESP_ERROR_CHECK(secure_storage_init());

    /* Record heap before the round-trip to verify no leaks */
    size_t heap_before = esp_get_free_heap_size();

    err = run_self_test();

    size_t heap_after = esp_get_free_heap_size();
    if (heap_before != heap_after) {
        /* NVS internal state may cause a small permanent change on first write. */
        ESP_LOGW(TAG, "Heap delta after round-trip: %d bytes (before=%zu after=%zu)",
                 (int)heap_before - (int)heap_after, heap_before, heap_after);
    }

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Result: PASS");
        led_blink_pass();
    } else {
        ESP_LOGE(TAG, "Result: FAIL");
        led_blink_fail();
    }

    run_benchmark();

    ESP_LOGI(TAG, "Done. See specs/FunctionalSpec.md for security notes.");
}
