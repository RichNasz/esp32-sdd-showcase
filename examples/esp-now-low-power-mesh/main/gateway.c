/* ================================================
   AGENT-GENERATED — DO NOT EDIT BY HAND
   Generated from specs/ using esp32-sdd-full-project-generator skill
   Date: 2026-04-09 | Agent: Claude Code
   ================================================ */

#include "gateway.h"
#include "mesh_common.h"
#include "mesh_types.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_now.h"
#include "esp_timer.h"
#include "esp_task_wdt.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <string.h>
#include <limits.h>
#include <stdio.h>

static const char *TAG = "gw";

static const uint8_t BROADCAST_MAC[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

/* ---------------------------------------------------------------------------
 * ANSI escape sequences
 *
 * All screen output goes through esp_rom_printf, which writes directly to
 * the UART ROM routine — bypassing newlib stdio and the VFS layer entirely,
 * guaranteeing every byte appears immediately with zero buffering.
 *
 * Packet lines and status content are pre-formatted with snprintf into char
 * buffers first, then output via esp_rom_printf("%s", buf). This avoids any
 * reliance on width/precision specifier support in the ROM printf.
 * ------------------------------------------------------------------------- */
#define ESC             "\033"
#define ANSI_CLEAR      ESC "[2J" ESC "[H"
#define ANSI_SAVE    "\0337"   /* DEC save cursor   (ESC 7) — universal, unlike SCO \033[s */
#define ANSI_RESTORE "\0338"   /* DEC restore cursor (ESC 8) — supported by all terminals */
#define ANSI_ERASE_EOL  ESC "[K"
#define ANSI_ROW1       ESC "[1;1H"
#define ANSI_ROW2       ESC "[2;1H"
#define ANSI_ROW3       ESC "[3;1H"
#define ANSI_ROW4       ESC "[4;1H"
#define ANSI_ROW5       ESC "[5;1H"
#define ANSI_ROW6       ESC "[6;1H"
/* Pin rows 1-5 as a fixed header; only rows 6+ scroll. */
#define ANSI_SCROLL_LOCK ESC "[6;500r"
#define ANSI_CURSOR_OFF  ESC "[?25l"  /* DECTCEM hide cursor */

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

/* ---------------------------------------------------------------------------
 * Packet log entry — stored in queue by recv_cb, printed by main loop
 * ------------------------------------------------------------------------- */
typedef struct {
    uint8_t  node_id;
    uint16_t msg_count;
    uint8_t  hop_count;
    int8_t   rssi[3];
    uint8_t  role;
    uint32_t uptime_s;
} packet_log_entry_t;

#define PACKET_QUEUE_DEPTH 8

/* ---------------------------------------------------------------------------
 * Dashboard state
 * (counters written by timer/recv tasks; screen drawn only by main loop)
 * ------------------------------------------------------------------------- */
static uint8_t  s_node_id;
static uint8_t  s_mac[6];
static uint32_t s_beacon_count  = 0;
static uint32_t s_packet_count  = 0;
static uint32_t s_last_beacon_s = 0; /* uptime-seconds of last good beacon */

static TaskHandle_t  s_main_task_handle;
static TaskHandle_t  s_led_task_handle;
static QueueHandle_t s_packet_queue;

/* ---------------------------------------------------------------------------
 * Screen drawing — called only from the main task to avoid cursor races
 * ------------------------------------------------------------------------- */

/* Redraw the status row (row 2). Saves and restores cursor so that the
 * main task's position in the scroll region is preserved. */
static void draw_status(void)
{
    uint32_t uptime_s = (uint32_t)(esp_timer_get_time() / 1000000ULL);
    uint32_t h   = uptime_s / 3600;
    uint32_t min = (uptime_s % 3600) / 60;
    uint32_t sec = uptime_s % 60;

    /* Pulse visible for 2 s after each beacon so the 1 Hz redraw catches it. */
    bool pulse = ((uptime_s - s_last_beacon_s) < 2);

    char status[200];
    snprintf(status, sizeof(status),
             "%s%s%s  beacon #%u  "
             C_GRAY "|" C_RESET "  "
             C_CYAN "up %02u:%02u:%02u" C_RESET "  "
             C_GRAY "|" C_RESET "  "
             "%u packet%s received",
             pulse ? C_GREEN : C_GRAY,
             pulse ? "[*]"   : "[ ]",
             C_RESET,
             (unsigned)s_beacon_count,
             (unsigned)h, (unsigned)min, (unsigned)sec,
             (unsigned)s_packet_count, s_packet_count == 1 ? "" : "s");

    /* Overwrite-then-erase: write content first so there is no visible blank
     * flash phase (ERASE before content causes an 8 ms blank window at 115200
     * baud while the new text fills in).  EOL erase after the content clears
     * any stale characters only at the right-hand tail. */
    esp_rom_printf(ANSI_SAVE ANSI_ROW2 " %s" ANSI_ERASE_EOL ANSI_RESTORE, status);
}

/* Print one packet log line to the scroll region.
 * Cursor is already in the scroll region; a trailing \n advances it. */
static void print_packet_line(const packet_log_entry_t *e)
{
    uint32_t h   = e->uptime_s / 3600;
    uint32_t min = (e->uptime_s % 3600) / 60;
    uint32_t sec = e->uptime_s % 60;

    bool is_sr  = (e->role == ESPNOW_ROLE_SENSOR_RELAY);
    bool hopped = (e->hop_count > 0);
    const char *role_col = is_sr  ? C_YELLOW : C_GREEN;
    const char *role_str = is_sr  ? "SR" : "S ";
    const char *path_str = hopped ? "relayed" : "direct ";

    const char *ssid_names[3] = {
        CONFIG_TARGET_SSID_1[0] ? CONFIG_TARGET_SSID_1 : "SSID1",
        CONFIG_TARGET_SSID_2[0] ? CONFIG_TARGET_SSID_2 : "SSID2",
        CONFIG_TARGET_SSID_3[0] ? CONFIG_TARGET_SSID_3 : "SSID3",
    };

    /* Build the three RSSI columns as fixed-width "NAME=VALUE" strings.
     * SSID name capped at 12 chars to match the header column width. */
    char rssi_col[3][20];
    const char *rssi_clr[3];
    for (int i = 0; i < 3; i++) {
        if (e->rssi[i] == INT8_MIN) {
            snprintf(rssi_col[i], sizeof(rssi_col[i]), "%.12s=---", ssid_names[i]);
            rssi_clr[i] = C_GRAY;
        } else {
            snprintf(rssi_col[i], sizeof(rssi_col[i]), "%.12s=%d", ssid_names[i], e->rssi[i]);
            rssi_clr[i] = e->rssi[i] > -60 ? C_GREEN
                        : e->rssi[i] > -80 ? C_YELLOW
                                            : C_RED;
        }
    }

    /* Pre-format the whole line so esp_rom_printf only needs "%s\n". */
    char line[300];
    int  pos = 0;

    pos += snprintf(line + pos, sizeof(line) - pos,
                    C_GRAY " %02u:%02u:%02u " C_RESET
                    "%s[%s/%s]" C_RESET "  "
                    "0x%02X  seq=%-5u hops=%u   ",
                    (unsigned)h, (unsigned)min, (unsigned)sec,
                    role_col, role_str, path_str,
                    e->node_id, (unsigned)e->msg_count, e->hop_count);

    for (int i = 0; i < 3; i++) {
        pos += snprintf(line + pos, sizeof(line) - pos,
                        "%s%-17s" C_RESET "  ",
                        rssi_clr[i], rssi_col[i]);
    }

    esp_rom_printf("%s\r\n", line);  /* CR returns cursor to col 0; LF advances row */
}

/* Draw the static header (rows 1-5) and set the scroll region to row 6+.
 * Called once after ESP-NOW init so startup log spam appears first,
 * then the screen is cleared and the dashboard takes over. */
static void draw_header(void)
{
    char s1[13], s2[13], s3[13];
    snprintf(s1, sizeof(s1), "%s", CONFIG_TARGET_SSID_1[0] ? CONFIG_TARGET_SSID_1 : "SSID1");
    snprintf(s2, sizeof(s2), "%s", CONFIG_TARGET_SSID_2[0] ? CONFIG_TARGET_SSID_2 : "SSID2");
    snprintf(s3, sizeof(s3), "%s", CONFIG_TARGET_SSID_3[0] ? CONFIG_TARGET_SSID_3 : "SSID3");

    char mac_str[18];
    snprintf(mac_str, sizeof(mac_str), MACSTR, MAC2STR(s_mac));

    esp_rom_printf(ANSI_CLEAR);

    /* Row 1 — title bar */
    char title[120];
    snprintf(title, sizeof(title),
             "  ESP-NOW MESH GATEWAY   ch:%-2d  node:0x%02X  mac:%s  ",
             CONFIG_ESPNOW_CHANNEL, s_node_id, mac_str);
    esp_rom_printf(ANSI_ROW1 C_BGBLU C_BOLD C_WHITE "%s" C_RESET "\n", title);

    /* Row 2 — status (populated by draw_status) */
    esp_rom_printf(ANSI_ROW2 "\n");

    /* Row 3 — separator */
    esp_rom_printf(ANSI_ROW3 C_GRAY
                   "------------------------------------------------------------------------"
                   "--------" C_RESET "\n");

    /* Row 4 — column headers */
    char hdr[160];
    snprintf(hdr, sizeof(hdr),
             "  uptime     role        node   seq     hops   %-17s  %-17s  %-17s",
             s1, s2, s3);
    esp_rom_printf(ANSI_ROW4 C_DIM "%s" C_RESET "\n", hdr);

    /* Row 5 — separator */
    esp_rom_printf(ANSI_ROW5 C_GRAY
                   "------------------------------------------------------------------------"
                   "--------" C_RESET "\n");

    /* Lock rows 1-5; cursor enters scroll region at row 6; hide cursor. */
    esp_rom_printf(ANSI_SCROLL_LOCK ANSI_ROW6 ANSI_CURSOR_OFF);

    draw_status();
}

/* ---------------------------------------------------------------------------
 * Beacon timer callback — updates state, wakes main loop via notification
 * ------------------------------------------------------------------------- */

static void beacon_timer_cb(void *arg)
{
    static uint8_t s_beacon_seq = 0;
    espnow_beacon_t beacon = { .seq = ++s_beacon_seq, .node_type = ESPNOW_NODE_GATEWAY };

    if (esp_now_send(BROADCAST_MAC, (uint8_t *)&beacon, sizeof(beacon)) == ESP_OK) {
        s_beacon_count++;
        s_last_beacon_s = (uint32_t)(esp_timer_get_time() / 1000000ULL);
        /* Wake main loop to refresh the status row. */
        if (s_main_task_handle) {
            xTaskNotifyGive(s_main_task_handle);
        }
    }
}

/* ---------------------------------------------------------------------------
 * ESP-NOW receive callback — enqueues packet, wakes main loop
 * ------------------------------------------------------------------------- */

static void gateway_recv_cb(const esp_now_recv_info_t *info,
                             const uint8_t *data, int data_len)
{
    if (data_len != (int)sizeof(espnow_payload_t)) {
        /* Ignore beacons and unknown packet sizes. */
        return;
    }

    const espnow_payload_t *payload = (const espnow_payload_t *)data;

    packet_log_entry_t entry = {
        .node_id   = payload->node_id,
        .msg_count = payload->msg_count,
        .hop_count = payload->hop_count,
        .rssi      = { payload->rssi[0], payload->rssi[1], payload->rssi[2] },
        .role      = payload->role,
        .uptime_s  = (uint32_t)(esp_timer_get_time() / 1000000ULL),
    };

    /* Non-blocking enqueue; drop if queue full (shouldn't happen at 60 s intervals). */
    xQueueSend(s_packet_queue, &entry, 0);

    /* Wake main loop and LED task. */
    if (s_main_task_handle) {
        xTaskNotifyGive(s_main_task_handle);
    }
    if (s_led_task_handle) {
        xTaskNotifyGive(s_led_task_handle);
    }
}

/* ---------------------------------------------------------------------------
 * Entry point
 * ------------------------------------------------------------------------- */

void gateway_run(void)
{
    mesh_wifi_init();
    mesh_led_init();

    s_node_id = mesh_node_id_get();
    mesh_node_mac_get(s_mac);
    ESP_LOGI(TAG, "node_id=0x%02X mac=" MACSTR " role=GATEWAY", s_node_id, MAC2STR(s_mac));

    mesh_espnow_set_channel(CONFIG_ESPNOW_CHANNEL);
    mesh_espnow_init();
    mesh_espnow_add_peer(BROADCAST_MAC);

    s_packet_queue = xQueueCreate(PACKET_QUEUE_DEPTH, sizeof(packet_log_entry_t));
    s_main_task_handle = xTaskGetCurrentTaskHandle();

    ESP_ERROR_CHECK(esp_now_register_recv_cb(gateway_recv_cb));

    /* Clear screen and draw TUI after init so startup log lines appear first. */
    draw_header();

    /* Suppress ESP-IDF system logs so they don't disturb the TUI. */
    esp_log_level_set("*", ESP_LOG_NONE);

    esp_timer_handle_t beacon_timer;
    const esp_timer_create_args_t timer_args = {
        .callback        = beacon_timer_cb,
        .arg             = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name            = "beacon",
    };
    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &beacon_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(beacon_timer,
                                             ESPNOW_BEACON_INTERVAL_MS * 1000LL));

    ESP_ERROR_CHECK(esp_task_wdt_add(NULL));

    xTaskCreate(mesh_led_task, "mesh_led", CONFIG_MESH_LED_TASK_STACK_SIZE,
                NULL, 2, &s_led_task_handle);

    /* Main loop: wakes on task notification (beacon or packet) or every 1 s.
     * All screen output happens here — single-task cursor ownership, no races. */
    while (1) {
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(1000));
        esp_task_wdt_reset();

        /* Drain the packet queue and print each entry into the scroll region. */
        packet_log_entry_t entry;
        while (xQueueReceive(s_packet_queue, &entry, 0) == pdTRUE) {
            s_packet_count++;
            print_packet_line(&entry);
        }

        /* Refresh the status row (save cursor → row 2 → update → restore). */
        draw_status();
    }
}
