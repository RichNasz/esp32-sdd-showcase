/* AGENT-GENERATED — DO NOT EDIT BY HAND
   Generated from specs/ using esp32-sdd-full-project-generator skill
   Date: 2026-03-11 | Agent: Claude Code
   Board: YEJMKJ ESP32-S3-DevKitC-1-N16R8
   ================================================ */

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_ota_ops.h"
#include "esp_https_ota.h"
#include "nvs_flash.h"
#include "driver/gpio.h"

/* ── Hardware pin assignments (board-specs/yejmkj/esp32-s3-devkitc-1-n16r8.md) ── */
#define GPIO_LED_LEFT      4   /* ota_0 active indicator — active-high, 3.3 V */
#define GPIO_LED_MIDDLE    5   /* OTA progress / error indicator — active-high */
#define GPIO_LED_RIGHT     6   /* ota_1 active indicator — active-high, 3.3 V */
#define GPIO_BUTTON        7   /* OTA trigger (press-to-GND) — active-low, pull-up */

/* ── Timing ────────────────────────────────────────────────────────────────── */
#define WIFI_CONNECT_TIMEOUT_MS    30000
#define HTTPS_OTA_TIMEOUT_MS       30000
#define ERROR_BLINK_HALF_PERIOD_MS  150

/* ── FreeRTOS event group bits ─────────────────────────────────────────────── */
#define WIFI_CONNECTED_BIT    BIT0
#define WIFI_FAIL_BIT         BIT1

static const char *TAG = "ota_main";

static EventGroupHandle_t s_wifi_event_group;
static SemaphoreHandle_t  s_button_sem;
static volatile bool      s_ota_running = false;

/* Embedded server CA certificate — see main/server_cert.pem */
extern const char server_cert_pem_start[] asm("_binary_server_cert_pem_start");
extern const char server_cert_pem_end[]   asm("_binary_server_cert_pem_end");

/* ────────────────────────────────────────────────────────────────────────────
   LED helpers
   ──────────────────────────────────────────────────────────────────────────── */

static void leds_init(void)
{
    const gpio_config_t cfg = {
        .pin_bit_mask = (1ULL << GPIO_LED_LEFT) |
                        (1ULL << GPIO_LED_MIDDLE) |
                        (1ULL << GPIO_LED_RIGHT),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&cfg));
    gpio_set_level(GPIO_LED_LEFT,   0);
    gpio_set_level(GPIO_LED_MIDDLE, 0);
    gpio_set_level(GPIO_LED_RIGHT,  0);
}

/* Light the left or right LED to show which OTA slot is active. */
static void led_set_status(void)
{
    const esp_partition_t *running = esp_ota_get_running_partition();
    if (running == NULL) {
        ESP_LOGW(TAG, "Cannot determine running partition — leaving LEDs off");
        return;
    }
    if (running->subtype == ESP_PARTITION_SUBTYPE_APP_OTA_0) {
        ESP_LOGI(TAG, "Running from ota_0 — left LED ON");
        gpio_set_level(GPIO_LED_LEFT,  1);
        gpio_set_level(GPIO_LED_RIGHT, 0);
    } else if (running->subtype == ESP_PARTITION_SUBTYPE_APP_OTA_1) {
        ESP_LOGI(TAG, "Running from ota_1 — right LED ON");
        gpio_set_level(GPIO_LED_LEFT,  0);
        gpio_set_level(GPIO_LED_RIGHT, 1);
    } else {
        ESP_LOGW(TAG, "Running from factory or unknown partition — both LEDs off");
        gpio_set_level(GPIO_LED_LEFT,  0);
        gpio_set_level(GPIO_LED_RIGHT, 0);
    }
}

/* Rapid triple-blink on middle LED — signals a non-recoverable failure. */
static void led_error_pattern(void)
{
    for (int i = 0; i < 6; i++) {
        gpio_set_level(GPIO_LED_MIDDLE, i % 2);
        vTaskDelay(pdMS_TO_TICKS(ERROR_BLINK_HALF_PERIOD_MS));
    }
    gpio_set_level(GPIO_LED_MIDDLE, 0);
}

/* ────────────────────────────────────────────────────────────────────────────
   OTA progress blink task
   Runs on a separate FreeRTOS task because app_main blocks during the OTA
   transfer — see CodingSpec.md Non-Functional Requirements.
   ──────────────────────────────────────────────────────────────────────────── */

static void ota_blink_task(void *arg)
{
    (void)arg;
    while (s_ota_running) {
        gpio_set_level(GPIO_LED_MIDDLE, 1);
        vTaskDelay(pdMS_TO_TICKS(300));
        gpio_set_level(GPIO_LED_MIDDLE, 0);
        vTaskDelay(pdMS_TO_TICKS(300));
    }
    gpio_set_level(GPIO_LED_MIDDLE, 0);
    vTaskDelete(NULL);
}

/* ────────────────────────────────────────────────────────────────────────────
   Button
   ──────────────────────────────────────────────────────────────────────────── */

static void IRAM_ATTR button_isr_handler(void *arg)
{
    (void)arg;
    BaseType_t higher_prio_woken = pdFALSE;
    xSemaphoreGiveFromISR(s_button_sem, &higher_prio_woken);
    if (higher_prio_woken) {
        portYIELD_FROM_ISR();
    }
}

static void button_init(void)
{
    const gpio_config_t cfg = {
        .pin_bit_mask = 1ULL << GPIO_BUTTON,
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_NEGEDGE,  /* falling edge = button pressed (active-low) */
    };
    ESP_ERROR_CHECK(gpio_config(&cfg));
    ESP_ERROR_CHECK(gpio_install_isr_service(0));
    ESP_ERROR_CHECK(gpio_isr_handler_add(GPIO_BUTTON, button_isr_handler, NULL));
}

/* ────────────────────────────────────────────────────────────────────────────
   Wi-Fi
   ──────────────────────────────────────────────────────────────────────────── */

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        /* No auto-reconnect — operator must observe the failure and reset */
        ESP_LOGW(TAG, "Wi-Fi disconnected");
        xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        const ip_event_got_ip_t *evt = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Wi-Fi connected — IP: " IPSTR, IP2STR(&evt->ip_info.ip));
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

static esp_err_t wifi_connect(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    const wifi_init_config_t init_cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&init_cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                               wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                               wifi_event_handler, NULL));

    wifi_config_t wifi_cfg = {0};
    strncpy((char *)wifi_cfg.sta.ssid,     CONFIG_OTA_WIFI_SSID,
            sizeof(wifi_cfg.sta.ssid) - 1);
    strncpy((char *)wifi_cfg.sta.password, CONFIG_OTA_WIFI_PASSWORD,
            sizeof(wifi_cfg.sta.password) - 1);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Wi-Fi connect: SSID=%s ...", CONFIG_OTA_WIFI_SSID);

    const EventBits_t bits = xEventGroupWaitBits(
        s_wifi_event_group,
        WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
        pdFALSE, pdFALSE,
        pdMS_TO_TICKS(WIFI_CONNECT_TIMEOUT_MS));

    if (bits & WIFI_CONNECTED_BIT) {
        return ESP_OK;
    }
    if (bits & WIFI_FAIL_BIT) {
        ESP_LOGE(TAG, "Wi-Fi connect failed (association rejected or AP not found)");
    } else {
        ESP_LOGE(TAG, "Wi-Fi connect timed out after %d ms", WIFI_CONNECT_TIMEOUT_MS);
    }
    return ESP_FAIL;
}

/* ────────────────────────────────────────────────────────────────────────────
   OTA download
   Uses esp_https_ota_begin/perform/finish for per-chunk progress logging.
   ──────────────────────────────────────────────────────────────────────────── */

static esp_err_t run_ota(void)
{
    const esp_http_client_config_t http_cfg = {
        .url              = CONFIG_OTA_SERVER_URL,
        .cert_pem         = server_cert_pem_start,
        .timeout_ms       = HTTPS_OTA_TIMEOUT_MS,
        .keep_alive_enable = true,
    };
    const esp_https_ota_config_t ota_cfg = {
        .http_config = &http_cfg,
    };

    esp_https_ota_handle_t handle = NULL;
    esp_err_t err = esp_https_ota_begin(&ota_cfg, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "OTA begin failed: %s", esp_err_to_name(err));
        return err;
    }

    const int img_size = esp_https_ota_get_image_size(handle);
    ESP_LOGI(TAG, "OTA image size: %d bytes", img_size);

    int last_logged_pct = -10;  /* ensures 0% is logged on first iteration */
    while (true) {
        err = esp_https_ota_perform(handle);
        if (err != ESP_ERR_HTTPS_OTA_IN_PROGRESS) {
            break;
        }
        if (img_size > 0) {
            const int read = esp_https_ota_get_image_len_read(handle);
            const int pct  = (read * 100) / img_size;
            if (pct >= last_logged_pct + 10) {
                ESP_LOGI(TAG, "OTA progress: %d%% (%d / %d bytes)", pct, read, img_size);
                last_logged_pct = pct;
            }
        }
    }

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "OTA perform failed: %s", esp_err_to_name(err));
        esp_https_ota_abort(handle);
        return err;
    }

    if (!esp_https_ota_is_complete_data_received(handle)) {
        ESP_LOGE(TAG, "OTA: incomplete data received — aborting");
        esp_https_ota_abort(handle);
        return ESP_FAIL;
    }

    err = esp_https_ota_finish(handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "OTA finish failed: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "OTA success — new image written to inactive partition");
    return ESP_OK;
}

/* ────────────────────────────────────────────────────────────────────────────
   app_main — linear OTA pipeline
   ──────────────────────────────────────────────────────────────────────────── */

void app_main(void)
{
    /* NVS must be initialised before Wi-Fi — see shared-specs/AIGenLessonsLearned.md */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS partition truncated or version changed — erasing and reinitialising");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    /* Hardware */
    leds_init();
    s_button_sem = xSemaphoreCreateBinary();

    /* ── Rollback check ────────────────────────────────────────────────────
       Unconditional and early — must happen before any user logic that could
       fail (see CodingSpec.md Gotchas). Failure to call this causes the
       bootloader to roll back on every subsequent reboot.
       ──────────────────────────────────────────────────────────────────── */
    {
        const esp_partition_t *running = esp_ota_get_running_partition();
        esp_ota_img_states_t ota_state = ESP_OTA_IMG_UNDEFINED;
        if (running && esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
            ESP_LOGI(TAG, "Rollback check: OTA image state = %d (%s)",
                     (int)ota_state,
                     ota_state == ESP_OTA_IMG_PENDING_VERIFY
                         ? "PENDING_VERIFY — marking valid now"
                         : "already confirmed");
        }
        ESP_ERROR_CHECK(esp_ota_mark_app_valid_cancel_rollback());
        ESP_LOGI(TAG, "Rollback check: app marked valid");
    }

    /* Show which OTA slot is active via left/right LED */
    led_set_status();

    /* ── Wi-Fi connect ─────────────────────────────────────────────────── */
    if (wifi_connect() != ESP_OK) {
        ESP_LOGE(TAG, "Wi-Fi failed — halting. Check credentials and reset to retry.");
        /* Halt without reboot — operator must observe the failure on the serial log */
        while (true) {
            led_error_pattern();
            vTaskDelay(pdMS_TO_TICKS(2000));
        }
    }

    /* Button init after Wi-Fi (gpio_install_isr_service must be called once) */
    button_init();
    ESP_LOGI(TAG, "Ready — press button (GPIO%d) to trigger OTA from: %s",
             GPIO_BUTTON, CONFIG_OTA_SERVER_URL);

    /* ── Wait for button press ─────────────────────────────────────────── */
    xSemaphoreTake(s_button_sem, portMAX_DELAY);
    ESP_LOGI(TAG, "Button pressed — starting OTA download");

    /* Start progress blink on a separate task (app_main will block on OTA) */
    s_ota_running = true;
    xTaskCreate(ota_blink_task, "ota_blink",
                CONFIG_OTA_LED_BLINK_TASK_STACK_SIZE, NULL, 3, NULL);

    /* ── HTTPS OTA ─────────────────────────────────────────────────────── */
    const esp_err_t ota_err = run_ota();
    s_ota_running = false;  /* signal blink task to stop */

    if (ota_err == ESP_OK) {
        ESP_LOGI(TAG, "OTA complete — rebooting into new firmware");
        vTaskDelay(pdMS_TO_TICKS(400));  /* allow blink task to exit cleanly */
        esp_restart();
    }

    /* ── OTA failure — halt without silent reboot ──────────────────────── */
    ESP_LOGE(TAG, "OTA failed (%s) — halting. No silent reboot. Check server and cert.",
             esp_err_to_name(ota_err));
    while (true) {
        led_error_pattern();
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
