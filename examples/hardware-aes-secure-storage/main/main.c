// ================================================
// AGENT-GENERATED — DO NOT EDIT BY HAND
// Generated from specs/ using esp32-sdd-full-project-generator skill
// Date: 2026-03-09 | Agent: Claude Code
// ================================================

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "secure_storage.h"

static const char *TAG = "aes_demo";

/* --------------------------------------------------------------------------
 * Board-agnostic LED configuration (values from Kconfig / sdkconfig.defaults)
 * -------------------------------------------------------------------------- */
#define LED_GPIO    CONFIG_EXAMPLE_LED_GPIO
#define LED_ON      CONFIG_EXAMPLE_LED_ACTIVE_LEVEL
#define LED_OFF     (1 - CONFIG_EXAMPLE_LED_ACTIVE_LEVEL)

/* --------------------------------------------------------------------------
 * Constants
 * -------------------------------------------------------------------------- */
#define BENCH_BYTES         (64 * 1024)       /* 64 KB benchmark payload */
#define BLINK_PASS_DELAY_MS 500               /* 1× slow blink on pass */
#define BLINK_FAIL_DELAY_MS 100               /* 3× fast blink on fail */
#define SELF_TEST_KEY       "selftest"        /* NVS key; max 15 chars  */
#define SELF_TEST_PLAINTEXT "Hello, AES-256-CBC hardware!"

/* --------------------------------------------------------------------------
 * LED helpers
 * -------------------------------------------------------------------------- */

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
