/* ================================================
   AGENT-GENERATED — DO NOT EDIT BY HAND
   Generated from specs/ using esp32-sdd-full-project-generator skill
   Date: 2026-04-12 | Agent: Claude Code
   ================================================ */

/*
 * BLE Beacon + Deep Sleep — multi-board (ESP32S3, ESP32C6, ESP32C5, ESP32)
 *
 * Stack choice: NimBLE (not Bluedroid). NimBLE initialises in ~50 ms vs ~200 ms
 * for Bluedroid, uses ~30 KB RAM vs ~60 KB, and provides a clean synchronous
 * nimble_port_stop() teardown path — all critical for a duty-cycled beacon
 * followed by immediate deep sleep.
 *
 * Cycle per wake:
 *   1. gpio_hold_dis -> determine wakeup cause -> increment RTC counters
 *   2. LED on -> NVS init -> BLE stack init -> start NimBLE host task
 *   3. Draw initial ANSI dashboard; suppress ESP-IDF logs
 *   4. sync_cb fires -> copy BLE MAC -> start non-connectable advertising
 *   5. 10 s esp_timer fires -> adv stop -> set s_adv_done flag
 *   6. app_main polling loop exits -> full BLE teardown -> LED off -> gpio_hold_en
 *   7. Configure sleep sources (10 s timer + GPIO button on S3/ESP32) -> esp_deep_sleep_start
 *
 * Dashboard: per-cycle snapshot pattern (cursor-home + overwrite every 250 ms).
 * See CodingSpec.md ## Monitor Dashboard and the esp32-ansi-monitor-engineer skill.
 *
 * BLE teardown sequence (3 calls — see shared-specs/ble-patterns.md):
 *   ble_gap_adv_stop() -> nimble_port_stop() -> nimble_port_deinit()
 *   nimble_port_deinit() handles controller disable/deinit internally on ESP-IDF 5.5.x.
 *   Do NOT call esp_bt_controller_disable/deinit explicitly — causes double-deinit crash.
 *
 * Board-specific configuration is resolved at compile time via Kconfig symbols
 * set in sdkconfig.defaults.<target>. Use `idf.py set-target <target>` to select.
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_timer.h"
#include "esp_task_wdt.h"
#include "esp_rom_sys.h"    /* esp_rom_printf() — zero-buffered ROM UART output */
#include "nvs_flash.h"

#include "driver/gpio.h"

#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/ble_gap.h"

/* -----------------------------------------------------------------------
 * Pin assignments — resolved from sdkconfig.defaults.<target> via Kconfig.
 * Values set in main/Kconfig.projbuild; overridden per target.
 * ----------------------------------------------------------------------- */
#define LED_GPIO        CONFIG_EXAMPLE_LED_GPIO
#define LED_ON          (CONFIG_EXAMPLE_LED_ACTIVE_LEVEL)
#define LED_OFF         (1 - CONFIG_EXAMPLE_LED_ACTIVE_LEVEL)
#define BUTTON_GPIO     CONFIG_EXAMPLE_BUTTON_GPIO

/* -----------------------------------------------------------------------
 * Timing
 * ----------------------------------------------------------------------- */
#define SLEEP_DURATION_US   (10ULL * 1000000ULL)   /* 10 s deep sleep period */
#define ADV_WINDOW_US       (10000ULL * 1000ULL)    /* 10 s advertising window */
#define DASH_REFRESH_US     (250ULL * 1000ULL)      /* 250 ms dashboard refresh */
#define DASH_BAR_WIDTH      20                      /* progress bar fill width */

static const char *TAG = "ble_beacon";

/* -----------------------------------------------------------------------
 * RTC-persistent state — survives deep sleep
 * ----------------------------------------------------------------------- */
static RTC_DATA_ATTR uint16_t boot_count = 0;
static RTC_DATA_ATTR uint16_t sequence   = 0;

/* -----------------------------------------------------------------------
 * ANSI escape sequences
 *
 * All post-init terminal output goes through esp_rom_printf(), which writes
 * directly to the UART ROM routine — bypassing newlib stdio and the VFS
 * layer entirely, guaranteeing every byte appears immediately with zero
 * buffering. Pre-format with snprintf into a stack buffer first, then emit
 * with esp_rom_printf("%s", buf) to avoid relying on ROM printf specifier
 * support (width/precision may be absent in some ESP-IDF ROM builds).
 *
 * Pattern B (per-cycle snapshot): cursor-home + overwrite every 250 ms.
 * No DECSTBM scroll region needed — the 6-row layout rewrites itself cleanly.
 * ANSI_SAVE/ANSI_RESTORE and ANSI_SCROLL_LOCK are Pattern A only (omitted).
 * ----------------------------------------------------------------------- */
#define ESC             "\033"

/* Cursor / screen */
#define ANSI_HOME       ESC "[H"            /* cursor to row 1, col 1 */
#define ANSI_CLEAR      ESC "[2J" ESC "[H"  /* clear screen + home (once on init) */
#define ANSI_ERASE_EOL  ESC "[K"            /* erase from cursor to end of line */
#define ANSI_CURSOR_OFF ESC "[?25l"         /* DECTCEM — hide cursor */

/* Colour / style */
#define C_RESET  ESC "[0m"
#define C_BOLD   ESC "[1m"
#define C_DIM    ESC "[2m"
#define C_GREEN  ESC "[32m"
#define C_YELLOW ESC "[33m"
#define C_RED    ESC "[31m"
#define C_CYAN   ESC "[36m"
#define C_GRAY   ESC "[90m"
#define C_WHITE  ESC "[97m"
#define C_BGBLU  ESC "[44m"

/* -----------------------------------------------------------------------
 * Dashboard state — written by callbacks, read only by draw_dashboard()
 * which runs exclusively on the app_main task.
 * ----------------------------------------------------------------------- */
typedef enum {
    ADV_STATUS_INIT        = 0,
    ADV_STATUS_ADVERTISING = 1,
    ADV_STATUS_SLEEPING    = 2,
} adv_status_t;

static volatile adv_status_t s_adv_status     = ADV_STATUS_INIT;
static volatile bool         s_adv_done        = false;
static uint8_t               s_ble_mac[6]      = {0};   /* NimBLE little-endian order */
static bool                  s_ble_mac_valid    = false;
static int64_t               s_cycle_start_us   = 0;    /* set when advertising begins */
static const char           *s_cause_str        = "FIRST_BOOT";
static TaskHandle_t          s_main_task_handle = NULL;

/* -----------------------------------------------------------------------
 * Module-scope handles — created fresh each wake cycle
 * ----------------------------------------------------------------------- */
static esp_timer_handle_t s_adv_timer;
static esp_timer_handle_t s_refresh_timer;

/* -----------------------------------------------------------------------
 * Forward declarations
 * ----------------------------------------------------------------------- */
static void ble_sync_cb(void);
static void ble_reset_cb(int reason);
static void ble_host_task(void *param);
static void adv_timer_cb(void *arg);
static void refresh_timer_cb(void *arg);
static void start_advertising(void);
static void ble_stack_teardown(void);
static void configure_sleep(void);
static void draw_dashboard(bool initial);

/* -----------------------------------------------------------------------
 * draw_dashboard — per-cycle snapshot pattern (Pattern B)
 *
 * Called every 250 ms by the polling loop and once with initial=true after
 * the NimBLE host task starts. Only called from the app_main task.
 *
 * Layout (6 rows, cursor-home + overwrite; no scroll region):
 *   Row 1: title bar
 *   Row 2: cycle number and wakeup cause
 *   Row 3: BLE MAC address
 *   Row 4: advertising status
 *   Row 5: active window progress bar
 *   Row 6: separator
 * ----------------------------------------------------------------------- */
static void draw_dashboard(bool initial)
{
    if (initial) {
        /* Clear screen once to remove ESP-IDF startup log spam.
         * All subsequent redraws use ANSI_HOME + overwrite to avoid flicker. */
        esp_rom_printf(ANSI_CLEAR ANSI_CURSOR_OFF);

        /* Suppress all ESP-IDF log output permanently. From this point, errors
         * and status must be shown via the Status field in the dashboard. */
        esp_log_level_set("*", ESP_LOG_NONE);
    }

    /* --- Compute progress bar --- */
    int filled = 0;
    float remain_s = 10.0f;
    if (s_cycle_start_us > 0) {
        int64_t elapsed_us = esp_timer_get_time() - s_cycle_start_us;
        if (elapsed_us < 0) elapsed_us = 0;
        float elapsed_s = (float)elapsed_us / 1000000.0f;
        remain_s = 10.0f - elapsed_s;
        if (remain_s < 0.0f) remain_s = 0.0f;
        int pct = (int)((elapsed_us * DASH_BAR_WIDTH) / (int64_t)ADV_WINDOW_US);
        filled = pct < 0 ? 0 : (pct > DASH_BAR_WIDTH ? DASH_BAR_WIDTH : pct);
    }
    if (s_adv_status == ADV_STATUS_SLEEPING) {
        filled = DASH_BAR_WIDTH;
        remain_s = 0.0f;
    }

    char bar[DASH_BAR_WIDTH + 3];  /* '[' + fill + ']' + NUL */
    bar[0] = '[';
    for (int i = 0; i < DASH_BAR_WIDTH; i++) {
        bar[i + 1] = (i < filled) ? '=' : ' ';
    }
    bar[DASH_BAR_WIDTH + 1] = ']';
    bar[DASH_BAR_WIDTH + 2] = '\0';

    /* --- Status string and colour --- */
    const char *status_str;
    const char *status_col;
    switch (s_adv_status) {
        case ADV_STATUS_INIT:
            status_str = "INITIALIZING"; status_col = C_YELLOW; break;
        case ADV_STATUS_ADVERTISING:
            status_str = "ADVERTISING";  status_col = C_GREEN;  break;
        case ADV_STATUS_SLEEPING:
            status_str = "SLEEPING";     status_col = C_GRAY;   break;
        default:
            status_str = "UNKNOWN";      status_col = C_RED;    break;
    }

    /* --- MAC string — "—" until ble_sync_cb populates s_ble_mac --- */
    char mac_str[20];
    if (s_ble_mac_valid) {
        /* NimBLE stores addresses little-endian; display MSB-first for readability */
        snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
                 s_ble_mac[5], s_ble_mac[4], s_ble_mac[3],
                 s_ble_mac[2], s_ble_mac[1], s_ble_mac[0]);
    } else {
        snprintf(mac_str, sizeof(mac_str), "--:--:--:--:--:--");
    }

    /* --- Row 1: title bar --- */
    char row1[100];
    snprintf(row1, sizeof(row1),
             "  BLE BEACON MONITOR   ESP32-SDD   cycle #%u  ",
             (unsigned)boot_count);
    esp_rom_printf(ANSI_HOME C_BGBLU C_BOLD C_WHITE "%s" C_RESET ANSI_ERASE_EOL "\n", row1);

    /* --- Row 2: cycle and wakeup cause --- */
    char row2[100];
    snprintf(row2, sizeof(row2),
             "  Cycle: " C_BOLD "#%-4u" C_RESET "   Wakeup: " C_CYAN "%s" C_RESET,
             (unsigned)boot_count, s_cause_str);
    esp_rom_printf("%s" ANSI_ERASE_EOL "\n", row2);

    /* --- Row 3: BLE MAC --- */
    char row3[100];
    snprintf(row3, sizeof(row3),
             "  BLE MAC: " C_BOLD "%s" C_RESET,
             mac_str);
    esp_rom_printf("%s" ANSI_ERASE_EOL "\n", row3);

    /* --- Row 4: advertising status --- */
    char row4[100];
    snprintf(row4, sizeof(row4),
             "  Status:  %s%s" C_RESET,
             status_col, status_str);
    esp_rom_printf("%s" ANSI_ERASE_EOL "\n", row4);

    /* --- Row 5: progress bar --- */
    char row5[100];
    snprintf(row5, sizeof(row5),
             "  Active:  " C_CYAN "%s" C_RESET "  %.1f s remaining",
             bar, (double)remain_s);
    esp_rom_printf("%s" ANSI_ERASE_EOL "\n", row5);

    /* --- Row 6: separator --- */
    esp_rom_printf(C_GRAY
                   "  ------------------------------------------------" C_RESET
                   ANSI_ERASE_EOL "\n");
}

/* -----------------------------------------------------------------------
 * Advertising
 *
 * Payload layout (26 bytes total — within the 31-byte PDU limit):
 *   [0x02 0x01 0x06]                     — Flags AD (3 bytes)
 *   [0x0A 0x09 "ESP32-SDD"]              — Complete Local Name AD (11 bytes)
 *   [0x0B 0xFF]                           — Mfr data AD header (2 bytes)
 *   [0xFF 0xFF]                           — Company ID 0xFFFF (2 bytes LE)
 *   [boot_lo boot_hi seq_lo seq_hi]       — boot_count + sequence (4 bytes LE)
 *   [0xDE 0xAD 0xBE 0xEF]                — Magic identifier (4 bytes)
 * ----------------------------------------------------------------------- */
static void start_advertising(void)
{
    uint8_t mfg_data[10];
    mfg_data[0] = 0xFF;
    mfg_data[1] = 0xFF;
    mfg_data[2] = (uint8_t)(boot_count);
    mfg_data[3] = (uint8_t)(boot_count >> 8);
    mfg_data[4] = (uint8_t)(sequence);
    mfg_data[5] = (uint8_t)(sequence >> 8);
    mfg_data[6] = 0xDE;
    mfg_data[7] = 0xAD;
    mfg_data[8] = 0xBE;
    mfg_data[9] = 0xEF;

    struct ble_hs_adv_fields fields = {0};
    fields.flags             = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    fields.name              = (uint8_t *)"ESP32-SDD";
    fields.name_len          = 9;
    fields.name_is_complete  = 1;
    fields.mfg_data          = mfg_data;
    fields.mfg_data_len      = sizeof(mfg_data);

    int rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0) {
        /* Reflect failure in Status field; signal main task to proceed to sleep */
        s_adv_status = ADV_STATUS_SLEEPING;
        s_adv_done   = true;
        xTaskNotifyGive(s_main_task_handle);
        return;
    }

    struct ble_gap_adv_params adv_params = {0};
    adv_params.conn_mode = BLE_GAP_CONN_MODE_NON;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_NON;
    adv_params.itvl_min  = 160;   /* 100 ms / 0.625 ms = 160 units */
    adv_params.itvl_max  = 160;

    rc = ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, NULL, BLE_HS_FOREVER,
                           &adv_params, NULL, NULL);
    if (rc != 0) {
        s_adv_status = ADV_STATUS_SLEEPING;
        s_adv_done   = true;
        xTaskNotifyGive(s_main_task_handle);
        return;
    }
}

/* Called by NimBLE host task when host and controller are synchronised.
 * Read MAC address, update dashboard state, start advertising. */
static void ble_sync_cb(void)
{
    uint8_t addr_val[6] = {0};
    int rc = ble_hs_id_copy_addr(BLE_ADDR_PUBLIC, addr_val, NULL);
    if (rc == 0) {
        memcpy(s_ble_mac, addr_val, 6);
        s_ble_mac_valid = true;
    }

    /* Transition to ADVERTISING status; dashboard will show on next refresh tick */
    s_adv_status = ADV_STATUS_ADVERTISING;
    xTaskNotifyGive(s_main_task_handle);   /* trigger immediate redraw */

    start_advertising();
}

/* Called if the NimBLE host resets unexpectedly. Signal main task to proceed to sleep. */
static void ble_reset_cb(int reason)
{
    (void)reason;
    s_adv_status = ADV_STATUS_SLEEPING;
    s_adv_done   = true;
    xTaskNotifyGive(s_main_task_handle);
}

/* NimBLE host task entry point. Blocks until nimble_port_stop() signals exit. */
static void ble_host_task(void *param)
{
    (void)param;
    nimble_port_run();
    nimble_port_freertos_deinit();
}

/* -----------------------------------------------------------------------
 * Advertising window timer callback — fires after 10 s advertising window
 *
 * Sets the advertising-complete flag and wakes the main task.
 * The main task's polling loop will draw a final SLEEPING frame and proceed.
 * ----------------------------------------------------------------------- */
static void adv_timer_cb(void *arg)
{
    (void)arg;
    ble_gap_adv_stop();
    s_adv_status = ADV_STATUS_SLEEPING;
    s_adv_done   = true;
    xTaskNotifyGive(s_main_task_handle);
}

/* Dashboard refresh timer callback — fires every 250 ms.
 * Only wakes the main task; all drawing is done on the main task. */
static void refresh_timer_cb(void *arg)
{
    (void)arg;
    xTaskNotifyGive(s_main_task_handle);
}

/* -----------------------------------------------------------------------
 * BLE stack teardown — MUST complete before esp_deep_sleep_start()
 *
 * 3-step sequence:
 *   1. ble_gap_adv_stop()  — idempotent; safe if already stopped by timer
 *   2. nimble_port_stop()  — signals host task to exit; blocks until done
 *   3. nimble_port_deinit() — frees NimBLE resources + controller disable/deinit
 *
 * In ESP-IDF 5.5.x, nimble_port_deinit() internally handles
 * esp_bt_controller_disable() and esp_bt_controller_deinit().
 * Do NOT call those APIs explicitly — causes double-deinit crash.
 * ----------------------------------------------------------------------- */
static void ble_stack_teardown(void)
{
    ble_gap_adv_stop();   /* idempotent — safe if already stopped */

    int rc = nimble_port_stop();
    if (rc != 0) {
        /* Log is suppressed — nothing to do here; teardown continues */
        (void)rc;
    }

    nimble_port_deinit();
}

/* -----------------------------------------------------------------------
 * Deep sleep wakeup source configuration
 *
 * Original ESP32 supports ext0 (single GPIO). RTC GPIOs: 0,2,4,12-15,25-27,32-39.
 * ESP32-S3: use ext1; GPIO 0-21 are all RTC-capable.
 * ESP32-C6: only GPIO 0-7 are RTC GPIOs. Boot button (GPIO 9) is NOT RTC-capable.
 * ESP32-C5: only GPIO 0-6 are RTC GPIOs. Boot button (GPIO 9) is NOT RTC-capable.
 * C6/C5 fall back to timer-only wakeup — no external hardware required.
 * ----------------------------------------------------------------------- */
static void configure_sleep(void)
{
    ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(SLEEP_DURATION_US));

#if CONFIG_IDF_TARGET_ESP32
    gpio_pullup_en(BUTTON_GPIO);
    gpio_pulldown_dis(BUTTON_GPIO);
    ESP_ERROR_CHECK(esp_sleep_enable_ext0_wakeup(BUTTON_GPIO, 0));
#elif CONFIG_IDF_TARGET_ESP32S3
    gpio_pullup_en(BUTTON_GPIO);
    gpio_pulldown_dis(BUTTON_GPIO);
    ESP_ERROR_CHECK(esp_sleep_enable_ext1_wakeup(1ULL << BUTTON_GPIO, ESP_EXT1_WAKEUP_ANY_LOW));
#else
    /* C6/C5: boot button GPIO 9 is not RTC-capable — timer-only wakeup */
    (void)BUTTON_GPIO;
#endif
}

/* -----------------------------------------------------------------------
 * app_main — runs once per wake cycle, then enters deep sleep
 * ----------------------------------------------------------------------- */
void app_main(void)
{
    /* Store main task handle immediately — callbacks may fire before we set it later */
    s_main_task_handle = xTaskGetCurrentTaskHandle();

    esp_task_wdt_add(NULL);
    esp_task_wdt_reset();

    /* Release GPIO hold from previous sleep cycle before driving any pin */
    gpio_hold_dis(LED_GPIO);

    /* ---- Wakeup cause ---- */
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    switch (cause) {
        case ESP_SLEEP_WAKEUP_TIMER:
            s_cause_str = "TIMER";
            break;
#if CONFIG_IDF_TARGET_ESP32
        case ESP_SLEEP_WAKEUP_EXT0:
            s_cause_str = "BUTTON (EXT0)";
            break;
#elif CONFIG_IDF_TARGET_ESP32S3
        case ESP_SLEEP_WAKEUP_EXT1:
            s_cause_str = "BUTTON (EXT1)";
            break;
#endif
        default:
            s_cause_str = "FIRST_BOOT";
            break;
    }

    /* ---- Increment RTC-persistent counters ---- */
    boot_count++;
    sequence++;
    esp_task_wdt_reset();

    /* ---- LED on ---- */
    gpio_config_t led_cfg = {
        .pin_bit_mask = (1ULL << LED_GPIO),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&led_cfg));
    gpio_set_level(LED_GPIO, LED_ON);

    /* ---- NVS init — required by BLE controller for PHY calibration data ---- */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    /* ---- Initialise NimBLE (controller + host) ----
     * nimble_port_init() handles esp_bt_controller_init/enable internally.
     * Do NOT call those APIs explicitly — causes double-init crash. */
    ESP_ERROR_CHECK(nimble_port_init());

    ble_hs_cfg.sync_cb  = ble_sync_cb;
    ble_hs_cfg.reset_cb = ble_reset_cb;

    /* Start NimBLE host task; sync_cb fires once host/controller are ready */
    nimble_port_freertos_init(ble_host_task);

    /* ---- Draw initial ANSI dashboard ----
     * Clears the screen (removing ESP-IDF startup log spam), hides cursor, and
     * suppresses all further ESP-IDF logging. Errors from this point are shown
     * in the Status field. Called after nimble_port_freertos_init() so startup
     * logs appear before the clear rather than after. */
    draw_dashboard(true);
    esp_task_wdt_reset();

    /* ---- Create 250 ms periodic dashboard refresh timer ---- */
    esp_timer_create_args_t refresh_args = {
        .callback = refresh_timer_cb,
        .name     = "dash_refresh",
    };
    ESP_ERROR_CHECK(esp_timer_create(&refresh_args, &s_refresh_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(s_refresh_timer, DASH_REFRESH_US));

    /* ---- Create and start 10 s advertising window timer ---- */
    esp_timer_create_args_t adv_timer_args = {
        .callback = adv_timer_cb,
        .name     = "adv_window",
    };
    ESP_ERROR_CHECK(esp_timer_create(&adv_timer_args, &s_adv_timer));

    /* Record cycle start time for progress bar (before starting adv timer) */
    s_cycle_start_us = esp_timer_get_time();
    ESP_ERROR_CHECK(esp_timer_start_once(s_adv_timer, ADV_WINDOW_US));

    /* Unsubscribe from WDT before the long polling wait (default WDT timeout is 5 s;
     * polling loop runs for up to 10 s while BLE is active) */
    esp_task_wdt_delete(NULL);

    /* ---- Polling loop — redraw dashboard every ~250 ms until adv window ends ---- */
    while (!s_adv_done) {
        /* Wait for either the refresh tick or an advertising-complete notification.
         * 300 ms timeout as a safety net in case a notification is missed. */
        xTaskNotifyWait(0, ULONG_MAX, NULL, pdMS_TO_TICKS(300));
        draw_dashboard(false);
    }
    /* Draw one final frame with SLEEPING status before tearing down */
    draw_dashboard(false);

    /* Re-subscribe for teardown phase */
    esp_task_wdt_add(NULL);
    esp_task_wdt_reset();

    /* ---- Clean up timers ---- */
    esp_timer_stop(s_refresh_timer);
    esp_timer_delete(s_refresh_timer);
    /* adv_timer may already be expired (ESP_ERR_INVALID_STATE is OK) */
    esp_timer_stop(s_adv_timer);
    esp_timer_delete(s_adv_timer);

    /* ---- Full BLE teardown — leaving radio powered destroys the power budget ---- */
    ble_stack_teardown();
    esp_task_wdt_reset();

    /* ---- LED off, hold GPIO state across deep sleep ---- */
    gpio_set_level(LED_GPIO, LED_OFF);
    gpio_hold_en(LED_GPIO);

    /* ---- Configure wakeup sources ---- */
    configure_sleep();

    /* Dashboard already shows SLEEPING; sleep immediately */
    esp_task_wdt_delete(NULL);
    esp_deep_sleep_start();
}
