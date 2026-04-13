/* AGENT-GENERATED — DO NOT EDIT BY HAND
   Generated from specs/ using esp32-sdd-full-project-generator skill
   Date: 2026-04-12 | Agent: Claude Code
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
#include "esp_rom_sys.h"

/* ── ANSI escape sequences (verbatim from esp32-ansi-monitor-engineer skill) ── */
#define ESC             "\033"
#define ANSI_HOME       ESC "[H"
#define ANSI_CLEAR      ESC "[2J" ESC "[H"
#define ANSI_ERASE_EOL  ESC "[K"
#define ANSI_CURSOR_OFF ESC "[?25l"
#define C_RESET         ESC "[0m"
#define C_BOLD          ESC "[1m"
#define C_DIM           ESC "[2m"
#define C_GREEN         ESC "[32m"
#define C_YELLOW        ESC "[33m"
#define C_RED           ESC "[31m"
#define C_CYAN          ESC "[36m"
#define C_GRAY          ESC "[90m"
#define C_WHITE         ESC "[97m"
#define C_BGBLU         ESC "[44m"

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

static const char *TAG = "ota_main";  /* used only before log suppression */

/* ── Dashboard status states ───────────────────────────────────────────────── */
typedef enum {
    OTA_STATUS_BOOTING,
    OTA_STATUS_WIFI_CONNECTING,
    OTA_STATUS_WIFI_FAILED,
    OTA_STATUS_READY,
    OTA_STATUS_DOWNLOADING,
    OTA_STATUS_COMPLETE,
    OTA_STATUS_ERROR,
} ota_status_t;

/* ── Module-scope dashboard state ──────────────────────────────────────────── */
static volatile ota_status_t s_ota_status    = OTA_STATUS_BOOTING;
static char   s_wifi_ip[16]                  = "";   /* "x.x.x.x" from event handler */
static char   s_error_reason[64]             = "";   /* populated on failure */
static int    s_img_size                     = 0;    /* total OTA image bytes */
static int    s_bytes_read                   = 0;    /* bytes written so far */
static int    s_wifi_elapsed_ms              = 0;    /* incremented each 500 ms poll */
static const char *s_partition_name          = "unknown";

static EventGroupHandle_t s_wifi_event_group;
static SemaphoreHandle_t  s_button_sem;
static volatile bool      s_ota_running = false;

/* Embedded server CA certificate — see main/server_cert.pem */
extern const char server_cert_pem_start[] asm("_binary_server_cert_pem_start");
extern const char server_cert_pem_end[]   asm("_binary_server_cert_pem_end");

/* ────────────────────────────────────────────────────────────────────────────
   ANSI dashboard — 7-row fixed layout (Pattern B: cursor-home + overwrite)

   Only app_main and functions called directly from app_main may call
   draw_dashboard(). The Wi-Fi event handler must not call draw_dashboard()
   or esp_rom_printf() — it sets event group bits only.
   ──────────────────────────────────────────────────────────────────────────── */

static void draw_dashboard(bool initial)
{
    if (initial) {
        esp_rom_printf(ANSI_CLEAR ANSI_CURSOR_OFF);
        esp_log_level_set("*", ESP_LOG_NONE);  /* permanent suppression */
    }

    char buf[256];

    esp_rom_printf(ANSI_HOME);

    /* Row 1: title bar */
    snprintf(buf, sizeof(buf),
             C_BOLD C_WHITE C_BGBLU "  OTA MONITOR   ESP32-SDD  "
             C_RESET ANSI_ERASE_EOL "\r\n");
    esp_rom_printf("%s", buf);

    /* Row 2: board and active partition */
    snprintf(buf, sizeof(buf),
             C_DIM "Board: " C_RESET "YEJMKJ ESP32-S3-DevKitC-1-N16R8"
             C_DIM "   Partition: " C_RESET "%s" ANSI_ERASE_EOL "\r\n",
             s_partition_name);
    esp_rom_printf("%s", buf);

    /* Row 3: OTA server URL (truncated to ~60 chars) */
    char url_buf[64];
    strncpy(url_buf, CONFIG_OTA_SERVER_URL, 60);
    url_buf[60] = '\0';
    if (strlen(CONFIG_OTA_SERVER_URL) > 60) {
        url_buf[57] = '.'; url_buf[58] = '.'; url_buf[59] = '.';
    }
    snprintf(buf, sizeof(buf),
             C_DIM "Server: " C_RESET "%s" ANSI_ERASE_EOL "\r\n", url_buf);
    esp_rom_printf("%s", buf);

    /* Row 4: Wi-Fi connection status */
    switch (s_ota_status) {
    case OTA_STATUS_WIFI_CONNECTING:
        snprintf(buf, sizeof(buf),
                 C_DIM "Wi-Fi:  " C_RESET C_YELLOW "CONNECTING (%d s)" C_RESET
                 ANSI_ERASE_EOL "\r\n",
                 s_wifi_elapsed_ms / 1000);
        break;
    case OTA_STATUS_WIFI_FAILED:
        snprintf(buf, sizeof(buf),
                 C_DIM "Wi-Fi:  " C_RESET C_RED "FAILED" C_RESET ANSI_ERASE_EOL "\r\n");
        break;
    default:
        if (s_wifi_ip[0] != '\0') {
            snprintf(buf, sizeof(buf),
                     C_DIM "Wi-Fi:  " C_RESET C_GREEN "CONNECTED (%s)" C_RESET
                     ANSI_ERASE_EOL "\r\n", s_wifi_ip);
        } else {
            snprintf(buf, sizeof(buf),
                     C_DIM "Wi-Fi:  " C_RESET "\xe2\x80\x94" ANSI_ERASE_EOL "\r\n");
        }
        break;
    }
    esp_rom_printf("%s", buf);

    /* Row 5: pipeline status */
    switch (s_ota_status) {
    case OTA_STATUS_BOOTING:
        snprintf(buf, sizeof(buf),
                 C_DIM "Status: " C_RESET C_CYAN "BOOTING" C_RESET ANSI_ERASE_EOL "\r\n");
        break;
    case OTA_STATUS_WIFI_CONNECTING:
        snprintf(buf, sizeof(buf),
                 C_DIM "Status: " C_RESET C_YELLOW "WIFI_CONNECTING" C_RESET
                 ANSI_ERASE_EOL "\r\n");
        break;
    case OTA_STATUS_WIFI_FAILED:
        snprintf(buf, sizeof(buf),
                 C_DIM "Status: " C_RESET C_RED "WIFI_FAILED" C_RESET ANSI_ERASE_EOL "\r\n");
        break;
    case OTA_STATUS_READY:
        snprintf(buf, sizeof(buf),
                 C_DIM "Status: " C_RESET C_GREEN "READY \xe2\x80\x94 press button" C_RESET
                 ANSI_ERASE_EOL "\r\n");
        break;
    case OTA_STATUS_DOWNLOADING:
        snprintf(buf, sizeof(buf),
                 C_DIM "Status: " C_RESET C_YELLOW "DOWNLOADING" C_RESET ANSI_ERASE_EOL "\r\n");
        break;
    case OTA_STATUS_COMPLETE:
        snprintf(buf, sizeof(buf),
                 C_DIM "Status: " C_RESET C_GREEN "COMPLETE" C_RESET ANSI_ERASE_EOL "\r\n");
        break;
    case OTA_STATUS_ERROR:
        snprintf(buf, sizeof(buf),
                 C_DIM "Status: " C_RESET C_RED "ERROR: %s" C_RESET ANSI_ERASE_EOL "\r\n",
                 s_error_reason);
        break;
    }
    esp_rom_printf("%s", buf);

    /* Row 6: progress bar — shown only while downloading with known image size */
    if (s_ota_status == OTA_STATUS_DOWNLOADING && s_img_size > 0) {
        const int pct  = (s_bytes_read * 100) / s_img_size;
        const int fill = pct / 5;   /* 20-char bar: 1 char per 5 % */
        char bar[21];
        for (int i = 0; i < 20; i++) bar[i] = (i < fill) ? '=' : ' ';
        bar[20] = '\0';
        snprintf(buf, sizeof(buf),
                 C_DIM "Progress:" C_RESET " [%s] %3d%%  %d / %d B" ANSI_ERASE_EOL "\r\n",
                 bar, pct, s_bytes_read, s_img_size);
    } else {
        snprintf(buf, sizeof(buf),
                 C_DIM "Progress:" C_RESET " \xe2\x80\x94" ANSI_ERASE_EOL "\r\n");
    }
    esp_rom_printf("%s", buf);

    /* Row 7: separator */
    snprintf(buf, sizeof(buf),
             C_GRAY "----------------------------------------" C_RESET ANSI_ERASE_EOL "\r\n");
    esp_rom_printf("%s", buf);
}

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

/* Light the left or right LED to show which OTA slot is active.
   Also sets s_partition_name for dashboard Row 2.
   Called before draw_dashboard(true) — ESP_LOGx is still active here. */
static void led_set_status(void)
{
    const esp_partition_t *running = esp_ota_get_running_partition();
    if (running == NULL) {
        ESP_LOGW(TAG, "Cannot determine running partition — leaving LEDs off");
        s_partition_name = "unknown";
        return;
    }
    if (running->subtype == ESP_PARTITION_SUBTYPE_APP_OTA_0) {
        ESP_LOGI(TAG, "Running from ota_0 — left LED ON");
        gpio_set_level(GPIO_LED_LEFT,  1);
        gpio_set_level(GPIO_LED_RIGHT, 0);
        s_partition_name = "ota_0";
    } else if (running->subtype == ESP_PARTITION_SUBTYPE_APP_OTA_1) {
        ESP_LOGI(TAG, "Running from ota_1 — right LED ON");
        gpio_set_level(GPIO_LED_LEFT,  0);
        gpio_set_level(GPIO_LED_RIGHT, 1);
        s_partition_name = "ota_1";
    } else {
        ESP_LOGW(TAG, "Running from factory or unknown partition — both LEDs off");
        gpio_set_level(GPIO_LED_LEFT,  0);
        gpio_set_level(GPIO_LED_RIGHT, 0);
        s_partition_name = "factory";
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
   Event handler: sets event group bits and s_wifi_ip only.
   Must not call draw_dashboard() or esp_rom_printf() — runs on a different task.
   ──────────────────────────────────────────────────────────────────────────── */

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        const ip_event_got_ip_t *evt = (ip_event_got_ip_t *)event_data;
        snprintf(s_wifi_ip, sizeof(s_wifi_ip), IPSTR, IP2STR(&evt->ip_info.ip));
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

/* Polling loop: waits up to 500 ms per iteration so draw_dashboard(false) can
   update elapsed time in Row 4 on each tick.
   Returns ESP_OK on connection, ESP_FAIL on timeout or rejected association. */
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

    s_ota_status      = OTA_STATUS_WIFI_CONNECTING;
    s_wifi_elapsed_ms = 0;
    draw_dashboard(false);

    int         total_elapsed_ms = 0;
    EventBits_t bits             = 0;

    while (total_elapsed_ms < WIFI_CONNECT_TIMEOUT_MS) {
        bits = xEventGroupWaitBits(s_wifi_event_group,
                                   WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                   pdFALSE, pdFALSE,
                                   pdMS_TO_TICKS(500));
        if (bits & (WIFI_CONNECTED_BIT | WIFI_FAIL_BIT)) {
            break;
        }
        s_wifi_elapsed_ms += 500;
        total_elapsed_ms  += 500;
        draw_dashboard(false);
    }

    if (bits & WIFI_CONNECTED_BIT) {
        return ESP_OK;
    }

    s_ota_status = OTA_STATUS_WIFI_FAILED;
    draw_dashboard(false);
    return ESP_FAIL;
}

/* ────────────────────────────────────────────────────────────────────────────
   OTA download
   Calls draw_dashboard(false) every ≥ 1 KB of progress to update the
   progress bar without flooding the UART on fast connections.
   On any failure, s_error_reason is populated before returning.
   ──────────────────────────────────────────────────────────────────────────── */

static esp_err_t run_ota(void)
{
    const esp_http_client_config_t http_cfg = {
        .url               = CONFIG_OTA_SERVER_URL,
        .cert_pem          = server_cert_pem_start,
        .timeout_ms        = HTTPS_OTA_TIMEOUT_MS,
        .keep_alive_enable = true,
    };
    const esp_https_ota_config_t ota_cfg = {
        .http_config = &http_cfg,
    };

    esp_https_ota_handle_t handle = NULL;
    esp_err_t err = esp_https_ota_begin(&ota_cfg, &handle);
    if (err != ESP_OK) {
        snprintf(s_error_reason, sizeof(s_error_reason),
                 "OTA begin: %s", esp_err_to_name(err));
        return err;
    }

    s_img_size   = esp_https_ota_get_image_size(handle);
    s_bytes_read = 0;
    int last_drawn_bytes = -1024;  /* force first dashboard draw */

    while (true) {
        err = esp_https_ota_perform(handle);
        if (err != ESP_ERR_HTTPS_OTA_IN_PROGRESS) {
            break;
        }
        s_bytes_read = esp_https_ota_get_image_len_read(handle);
        if (s_bytes_read - last_drawn_bytes >= 1024) {
            draw_dashboard(false);
            last_drawn_bytes = s_bytes_read;
        }
    }

    if (err != ESP_OK) {
        snprintf(s_error_reason, sizeof(s_error_reason),
                 "OTA perform: %s", esp_err_to_name(err));
        esp_https_ota_abort(handle);
        return err;
    }

    if (!esp_https_ota_is_complete_data_received(handle)) {
        snprintf(s_error_reason, sizeof(s_error_reason), "incomplete data received");
        esp_https_ota_abort(handle);
        return ESP_FAIL;
    }

    err = esp_https_ota_finish(handle);
    if (err != ESP_OK) {
        snprintf(s_error_reason, sizeof(s_error_reason),
                 "OTA finish: %s", esp_err_to_name(err));
        return err;
    }

    return ESP_OK;
}

/* ────────────────────────────────────────────────────────────────────────────
   app_main — linear OTA pipeline

   Phase order:
     NVS init → LED init → rollback check → led_set_status()
     → draw_dashboard(true) [log suppression begins here]
     → wifi_connect() → button_init() → wait for button
     → run_ota() → restart (success) or error loop (failure)
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

    /* Show which OTA slot is active via left/right LED.
       Also sets s_partition_name for the dashboard.
       ESP_LOGx is still active at this point. */
    led_set_status();

    /* Clear the terminal, suppress ESP-IDF logs permanently, show BOOTING.
       All diagnostic output from this point forward is via draw_dashboard(). */
    draw_dashboard(true);

    /* ── Wi-Fi connect (polling loop — dashboard refreshes each 500 ms tick) ── */
    if (wifi_connect() != ESP_OK) {
        /* s_ota_status is OTA_STATUS_WIFI_FAILED, already drawn in wifi_connect() */
        while (true) {
            led_error_pattern();
            vTaskDelay(pdMS_TO_TICKS(2000));
        }
    }

    /* Button init after Wi-Fi (gpio_install_isr_service must be called once) */
    button_init();

    s_ota_status = OTA_STATUS_READY;
    draw_dashboard(false);

    /* ── Wait for button press ─────────────────────────────────────────── */
    xSemaphoreTake(s_button_sem, portMAX_DELAY);

    /* Start progress blink on a separate task (app_main will block on OTA) */
    s_ota_status  = OTA_STATUS_DOWNLOADING;
    s_ota_running = true;
    xTaskCreate(ota_blink_task, "ota_blink",
                CONFIG_OTA_LED_BLINK_TASK_STACK_SIZE, NULL, 3, NULL);
    draw_dashboard(false);

    /* ── HTTPS OTA ─────────────────────────────────────────────────────── */
    const esp_err_t ota_err = run_ota();
    s_ota_running = false;  /* signal blink task to stop */

    if (ota_err == ESP_OK) {
        s_ota_status = OTA_STATUS_COMPLETE;
        draw_dashboard(false);
        vTaskDelay(pdMS_TO_TICKS(400));  /* allow blink task to exit cleanly */
        esp_restart();
    }

    /* ── OTA failure — halt without silent reboot ──────────────────────── */
    /* run_ota() populates s_error_reason on failure; guard against empty string */
    if (s_error_reason[0] == '\0') {
        snprintf(s_error_reason, sizeof(s_error_reason), "%s", esp_err_to_name(ota_err));
    }
    s_ota_status = OTA_STATUS_ERROR;
    draw_dashboard(false);
    while (true) {
        led_error_pattern();
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
