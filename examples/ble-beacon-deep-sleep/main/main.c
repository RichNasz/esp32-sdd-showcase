/* ================================================
   AGENT-GENERATED — DO NOT EDIT BY HAND
   Generated from specs/ using esp32-sdd-full-project-generator skill
   Date: 2026-04-12 | Agent: Claude Code
   ================================================ */

/*
 * BLE Beacon + Deep Sleep — multi-board (ESP32, ESP32-S3, ESP32-C5, ESP32-C6)
 *
 * Two LED code paths, selected at compile time by CONFIG_EXAMPLE_LED_WS2812:
 *   WS2812 path (YEJMKJ DevKitC GPIO 48, ESP32-C6-DevKitC GPIO 8):
 *     Solid blue (0, 0, 64) while BLE advertising is active; dark otherwise.
 *   GPIO path (HUZZAH32 GPIO 13, XIAO variants):
 *     Active level while advertising; inactive at all other times.
 *   In both paths the LED is dark during NimBLE init and teardown phases —
 *   it indicates actual BLE TX activity, not the overall active window.
 *
 * Dashboard: per-cycle snapshot pattern (cursor-home + overwrite every 250 ms).
 * See CodingSpec.md ## Monitor Dashboard and the esp32-ansi-monitor-engineer skill.
 *
 * Cycle per wake:
 *   1. gpio_hold_dis -> determine wakeup cause -> increment RTC counters
 *   2. led_init (hardware only; LED stays dark) -> NVS init -> NimBLE init
 *   3. Draw initial ANSI dashboard; suppress ESP-IDF logs
 *   4. sync_cb fires -> copy BLE MAC -> led_adv_on -> start advertising
 *   5. 10 s adv_timer fires -> ble_gap_adv_stop -> led_adv_off -> set done flag
 *   6. Polling loop exits -> teardown -> gpio_hold_en (GPIO path) -> deep sleep
 *
 * BLE teardown: ble_gap_adv_stop() -> nimble_port_stop() -> nimble_port_deinit()
 * nimble_port_deinit() handles controller disable/deinit internally (ESP-IDF 5.5.x).
 * Do NOT call esp_bt_controller_disable/deinit explicitly — double-deinit crash.
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

#if CONFIG_EXAMPLE_LED_WS2812
#include "driver/rmt_tx.h"
#include "driver/rmt_encoder.h"
#endif

#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/ble_gap.h"

/* -----------------------------------------------------------------------
 * Pin assignments — resolved from sdkconfig.defaults.<target> via Kconfig.
 * ----------------------------------------------------------------------- */
#define LED_GPIO    CONFIG_EXAMPLE_LED_GPIO
#define LED_ON      (CONFIG_EXAMPLE_LED_ACTIVE_LEVEL)
#define LED_OFF     (1 - CONFIG_EXAMPLE_LED_ACTIVE_LEVEL)
#define BUTTON_GPIO CONFIG_EXAMPLE_BUTTON_GPIO

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

/* ================================================================
   WS2812 RMT path — compiled when CONFIG_EXAMPLE_LED_WS2812=y
   Generated per esp32-ws2812-led-engineer skill workflow.
   LED colour = solid blue (0, 0, 64) during BLE advertising.
   64/255 is the minimum brightness floor per AIGenLessonsLearned.md.
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

static void led_init(void)
{
    gpio_hold_dis(LED_GPIO);  /* release any hold set by the previous sleep cycle */

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

/* Blue (0, 0, 64) — BLE advertising active. 64/255 is the minimum visible floor. */
static void led_adv_on(void)  { ws2812_write(0, 0, 64); }
static void led_adv_off(void) { ws2812_write(0, 0, 0);  }
/* RMT eot_level=0 holds data line LOW after each frame — gpio_hold_en() not needed. */

/* ================================================================
   Simple GPIO path — compiled when CONFIG_EXAMPLE_LED_WS2812=n
   ================================================================ */
#else

static void led_init(void)
{
    gpio_hold_dis(LED_GPIO);  /* release any hold set by the previous sleep cycle */

    const gpio_config_t io_cfg = {
        .pin_bit_mask = (1ULL << LED_GPIO),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&io_cfg));
    gpio_set_level(LED_GPIO, LED_OFF);  /* inactive = LED off; stays dark during BLE init */
}

static void led_adv_on(void)  { gpio_set_level(LED_GPIO, LED_ON);  }
static void led_adv_off(void) { gpio_set_level(LED_GPIO, LED_OFF); }
/* gpio_hold_en() called in app_main just before deep sleep — not here,
 * because the device may still be active for some time after advertising stops. */

#endif  /* CONFIG_EXAMPLE_LED_WS2812 */

/* -----------------------------------------------------------------------
 * ANSI escape sequences — per-cycle snapshot pattern (Pattern B)
 * All post-init terminal output goes through esp_rom_printf(), which writes
 * directly to the UART ROM routine — bypassing newlib stdio and VFS entirely.
 * Pre-format with snprintf into a stack buffer; emit with esp_rom_printf("%s", buf).
 * ----------------------------------------------------------------------- */
#define ESC             "\033"
#define ANSI_HOME       ESC "[H"
#define ANSI_CLEAR      ESC "[2J" ESC "[H"
#define ANSI_ERASE_EOL  ESC "[K"
#define ANSI_CURSOR_OFF ESC "[?25l"

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

static volatile adv_status_t s_adv_status      = ADV_STATUS_INIT;
static volatile bool         s_adv_done         = false;
static uint8_t               s_ble_mac[6]       = {0};
static bool                  s_ble_mac_valid     = false;
static int64_t               s_cycle_start_us    = 0;
static const char           *s_cause_str         = "FIRST_BOOT";
static TaskHandle_t          s_main_task_handle  = NULL;

/* -----------------------------------------------------------------------
 * Module-scope timer handles — created fresh each wake cycle
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
 * Called every ~250 ms during the advertising window and once with
 * initial=true after NimBLE starts. Only called from the app_main task.
 * ----------------------------------------------------------------------- */
static void draw_dashboard(bool initial)
{
    if (initial) {
        esp_rom_printf(ANSI_CLEAR ANSI_CURSOR_OFF);
        esp_log_level_set("*", ESP_LOG_NONE);
    }

    /* --- Progress bar --- */
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

    char bar[DASH_BAR_WIDTH + 3];
    bar[0] = '[';
    for (int i = 0; i < DASH_BAR_WIDTH; i++) bar[i + 1] = (i < filled) ? '=' : ' ';
    bar[DASH_BAR_WIDTH + 1] = ']';
    bar[DASH_BAR_WIDTH + 2] = '\0';

    /* --- Status string and colour --- */
    const char *status_str;
    const char *status_col;
    switch (s_adv_status) {
        case ADV_STATUS_INIT:        status_str = "INITIALIZING"; status_col = C_YELLOW; break;
        case ADV_STATUS_ADVERTISING: status_str = "ADVERTISING";  status_col = C_GREEN;  break;
        case ADV_STATUS_SLEEPING:    status_str = "SLEEPING";     status_col = C_GRAY;   break;
        default:                     status_str = "UNKNOWN";      status_col = C_RED;    break;
    }

    /* --- MAC string --- */
    char mac_str[20];
    if (s_ble_mac_valid) {
        snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
                 s_ble_mac[5], s_ble_mac[4], s_ble_mac[3],
                 s_ble_mac[2], s_ble_mac[1], s_ble_mac[0]);
    } else {
        snprintf(mac_str, sizeof(mac_str), "--:--:--:--:--:--");
    }

    /* Row 1 — title bar */
    char row1[100];
    snprintf(row1, sizeof(row1),
             "  BLE BEACON MONITOR   ESP32-SDD   cycle #%u  ",
             (unsigned)boot_count);
    esp_rom_printf(ANSI_HOME C_BGBLU C_BOLD C_WHITE "%s" C_RESET ANSI_ERASE_EOL "\n", row1);

    /* Row 2 — cycle and wakeup cause */
    char row2[100];
    snprintf(row2, sizeof(row2),
             "  Cycle: " C_BOLD "#%-4u" C_RESET "   Wakeup: " C_CYAN "%s" C_RESET,
             (unsigned)boot_count, s_cause_str);
    esp_rom_printf("%s" ANSI_ERASE_EOL "\n", row2);

    /* Row 3 — BLE MAC */
    char row3[100];
    snprintf(row3, sizeof(row3), "  BLE MAC: " C_BOLD "%s" C_RESET, mac_str);
    esp_rom_printf("%s" ANSI_ERASE_EOL "\n", row3);

    /* Row 4 — advertising status */
    char row4[100];
    snprintf(row4, sizeof(row4), "  Status:  %s%s" C_RESET, status_col, status_str);
    esp_rom_printf("%s" ANSI_ERASE_EOL "\n", row4);

    /* Row 5 — progress bar */
    char row5[100];
    snprintf(row5, sizeof(row5),
             "  Active:  " C_CYAN "%s" C_RESET "  %.1f s remaining",
             bar, (double)remain_s);
    esp_rom_printf("%s" ANSI_ERASE_EOL "\n", row5);

    /* Row 6 — separator */
    esp_rom_printf(C_GRAY
                   "  ------------------------------------------------" C_RESET
                   ANSI_ERASE_EOL "\n");
}

/* -----------------------------------------------------------------------
 * Advertising payload
 *
 * Layout (26 bytes total — within the 31-byte PDU limit):
 *   Flags (3) + Complete Local Name "ESP32-SDD" (11) + Mfr data (12) = 26 bytes
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

    /* Advertising is now active — turn on the LED to indicate BLE TX */
    led_adv_on();
}

/* Called by NimBLE host task when host and controller are synchronised.
 * Reads MAC, updates dashboard state, then starts advertising (and LED). */
static void ble_sync_cb(void)
{
    uint8_t addr_val[6] = {0};
    if (ble_hs_id_copy_addr(BLE_ADDR_PUBLIC, addr_val, NULL) == 0) {
        memcpy(s_ble_mac, addr_val, 6);
        s_ble_mac_valid = true;
    }

    s_adv_status = ADV_STATUS_ADVERTISING;
    xTaskNotifyGive(s_main_task_handle);   /* trigger immediate dashboard redraw */

    start_advertising();
}

/* Called if the NimBLE host resets unexpectedly. Turn off LED and signal done. */
static void ble_reset_cb(int reason)
{
    (void)reason;
    led_adv_off();   /* LED may be on if reset fired after advertising started */
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
 * Advertising window timer — fires after 10 s
 * Stops advertising, turns off LED, sets done flag.
 * ----------------------------------------------------------------------- */
static void adv_timer_cb(void *arg)
{
    (void)arg;
    ble_gap_adv_stop();
    led_adv_off();   /* turn off LED immediately when advertising stops */
    s_adv_status = ADV_STATUS_SLEEPING;
    s_adv_done   = true;
    xTaskNotifyGive(s_main_task_handle);
}

/* Dashboard 250 ms refresh timer — wakes main task only; no LED or screen writes here. */
static void refresh_timer_cb(void *arg)
{
    (void)arg;
    xTaskNotifyGive(s_main_task_handle);
}

/* -----------------------------------------------------------------------
 * BLE stack teardown
 *
 * 3-step sequence required before deep sleep:
 *   ble_gap_adv_stop()   — idempotent (advertising already stopped by timer)
 *   nimble_port_stop()   — signals host task to exit; blocks until done
 *   nimble_port_deinit() — frees NimBLE + handles controller disable/deinit
 *
 * In ESP-IDF 5.5.x, nimble_port_deinit() internally calls
 * esp_bt_controller_disable() and esp_bt_controller_deinit().
 * Do NOT call those APIs explicitly — causes double-deinit crash.
 * ----------------------------------------------------------------------- */
static void ble_stack_teardown(void)
{
    ble_gap_adv_stop();   /* idempotent — advertising already stopped by timer */
    nimble_port_stop();   /* blocks until host task exits */
    nimble_port_deinit();
}

/* -----------------------------------------------------------------------
 * Deep sleep wakeup source configuration
 *
 * ESP32-S3: GPIO 0–21 all RTC-capable → ext1 button wakeup on GPIO 0.
 * ESP32 (original): GPIO 0 is RTC-capable (RTCIO_CH11) → ext0 button wakeup.
 * ESP32-C6: only GPIO 0–7 are RTC-capable. Boot button is GPIO 9 → not RTC.
 * ESP32-C5: only GPIO 0–6 are RTC-capable. Boot button is GPIO 9 → not RTC.
 * YEJMKJ DevKitC: boot button GPIO not RTC-capable on this variant → timer-only.
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
    (void)BUTTON_GPIO;
    /* C6/C5 and YEJMKJ DevKitC: timer-only wakeup */
#endif
}

/* -----------------------------------------------------------------------
 * app_main — runs once per wake cycle, then enters deep sleep
 * ----------------------------------------------------------------------- */
void app_main(void)
{
    /* Store main task handle first — callbacks may reference it before we continue */
    s_main_task_handle = xTaskGetCurrentTaskHandle();

    esp_task_wdt_add(NULL);
    esp_task_wdt_reset();

    /* ---- Wakeup cause ---- */
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    switch (cause) {
        case ESP_SLEEP_WAKEUP_TIMER: s_cause_str = "TIMER";          break;
#if CONFIG_IDF_TARGET_ESP32
        case ESP_SLEEP_WAKEUP_EXT0:  s_cause_str = "BUTTON (EXT0)";  break;
#elif CONFIG_IDF_TARGET_ESP32S3
        case ESP_SLEEP_WAKEUP_EXT1:  s_cause_str = "BUTTON (EXT1)";  break;
#endif
        default:                     s_cause_str = "FIRST_BOOT";     break;
    }

    /* ---- Increment RTC-persistent counters ---- */
    boot_count++;
    sequence++;
    esp_task_wdt_reset();

    /* ---- LED hardware init — dark start; LED turns on when advertising begins ----
     * Both paths call gpio_hold_dis() internally. The LED is initialised to its
     * inactive state here and will only light up inside start_advertising(). */
    led_init();

    /* ---- NVS init — required before BLE controller init ---- */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    /* ---- Initialise NimBLE (controller + host) ----
     * nimble_port_init() handles esp_bt_controller_init/enable internally. */
    ESP_ERROR_CHECK(nimble_port_init());

    ble_hs_cfg.sync_cb  = ble_sync_cb;
    ble_hs_cfg.reset_cb = ble_reset_cb;

    /* Start NimBLE host task; sync_cb fires once host/controller are ready */
    nimble_port_freertos_init(ble_host_task);

    /* ---- Draw initial ANSI dashboard ----
     * Clears screen, hides cursor, suppresses all subsequent ESP-IDF logs.
     * Called after nimble_port_freertos_init() so startup logs appear before
     * the screen clear rather than intermixed with dashboard content. */
    draw_dashboard(true);
    esp_task_wdt_reset();

    /* ---- Create 250 ms dashboard refresh timer ---- */
    esp_timer_create_args_t refresh_args = {
        .callback = refresh_timer_cb,
        .name     = "dash_refresh",
    };
    ESP_ERROR_CHECK(esp_timer_create(&refresh_args, &s_refresh_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(s_refresh_timer, DASH_REFRESH_US));

    /* ---- Create and start 10 s advertising window timer ---- */
    esp_timer_create_args_t adv_args = {
        .callback = adv_timer_cb,
        .name     = "adv_window",
    };
    ESP_ERROR_CHECK(esp_timer_create(&adv_args, &s_adv_timer));

    s_cycle_start_us = esp_timer_get_time();
    ESP_ERROR_CHECK(esp_timer_start_once(s_adv_timer, ADV_WINDOW_US));

    /* Unsubscribe from WDT before the 10 s polling wait (default timeout is 5 s) */
    esp_task_wdt_delete(NULL);

    /* ---- Polling loop — redraw dashboard every ~250 ms ---- */
    while (!s_adv_done) {
        xTaskNotifyWait(0, ULONG_MAX, NULL, pdMS_TO_TICKS(300));
        draw_dashboard(false);
    }
    draw_dashboard(false);  /* final SLEEPING frame */

    /* Re-subscribe for teardown */
    esp_task_wdt_add(NULL);
    esp_task_wdt_reset();

    /* ---- Clean up timers ---- */
    esp_timer_stop(s_refresh_timer);
    esp_timer_delete(s_refresh_timer);
    esp_timer_stop(s_adv_timer);   /* may already be expired — error ignored */
    esp_timer_delete(s_adv_timer);

    /* ---- Full BLE teardown ---- */
    ble_stack_teardown();
    esp_task_wdt_reset();

    /* Safety: ensure LED is off regardless of which path ended advertising.
     * Covers the ble_reset_cb() error path where adv_timer_cb() never fired. */
    led_adv_off();

    /* ---- Hold GPIO state for deep sleep (GPIO path only) ----
     * The WS2812 RMT holds its data line LOW via eot_level=0 — no gpio_hold_en() needed.
     * The GPIO path must hold the inactive level explicitly so the pin does not float. */
#if !CONFIG_EXAMPLE_LED_WS2812
    gpio_hold_en(LED_GPIO);
#endif

    /* ---- Configure wakeup sources and sleep ---- */
    configure_sleep();
    esp_task_wdt_delete(NULL);
    esp_deep_sleep_start();
}
