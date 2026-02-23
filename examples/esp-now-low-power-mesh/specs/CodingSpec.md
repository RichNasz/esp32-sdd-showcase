# ==================== BOARD SELECTION ====================
# Change only this section, then regenerate the project
#
# Board: seeed-xiao-esp32s3          ← ACTIVE / DEFAULT
#
# =======================================================
# Institutional Memory: shared-specs/AIGenLessonsLearned.md — read before regenerating

# Coding Specification — ESP-NOW Low-Power Mesh

## Role Selection via Kconfig

```
choice NODE_ROLE
    prompt "Node role"
    default NODE_ROLE_SENSOR

    config NODE_ROLE_SENSOR
        bool "Sensor node (deep sleep between sends)"

    config NODE_ROLE_GATEWAY
        bool "Gateway node (always-on receiver)"
endchoice

config GATEWAY_MAC
    string "Gateway MAC address (AA:BB:CC:DD:EE:FF format)"
    default "FF:FF:FF:FF:FF:FF"
    depends on NODE_ROLE_SENSOR

config NODE_ID
    int "This node's ID (1-255)"
    default 1
    range 1 255
```

Place in `main/Kconfig.projbuild`.

## ESP-NOW Initialisation (both roles)

```c
// Phase 1: Wi-Fi init (required for ESP-NOW)
esp_netif_init();
esp_event_loop_create_default();
wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
esp_wifi_init(&cfg);
esp_wifi_set_mode(WIFI_MODE_STA);
esp_wifi_start();
esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);  // fixed channel

// Phase 2: ESP-NOW init
esp_now_init();
```

## Sensor Node (`#if CONFIG_NODE_ROLE_SENSOR`)

- RTC memory: `static RTC_DATA_ATTR uint16_t msg_count = 0;`
- Deep sleep: `esp_sleep_enable_timer_wakeup(20 * 1000000ULL)`.
- `#define SLEEP_SEC 20`
- `#define SEND_TIMEOUT_MS 200`

Send sequence:
1. Parse `CONFIG_GATEWAY_MAC` string into `uint8_t mac[6]` using `sscanf`.
2. Register peer: `esp_now_add_peer()` with gateway MAC, channel 1, no encryption.
3. Build `espnow_payload_t` struct.
4. Register send callback: `esp_now_register_send_cb(on_send_cb)`.
5. `esp_now_send(gateway_mac, (uint8_t*)&payload, sizeof(payload))`.
6. Wait on `xSemaphoreTake(send_done_sem, pdMS_TO_TICKS(SEND_TIMEOUT_MS))`.
7. Blink LED based on `esp_now_send_status_t` from callback.
8. `esp_now_deinit()` → `esp_wifi_stop()` → `esp_deep_sleep_start()`.

## Gateway Node (`#if CONFIG_NODE_ROLE_GATEWAY`)

- No deep sleep; runs `while(1) { vTaskDelay(pdMS_TO_TICKS(1000)); }`.
- `esp_now_register_recv_cb(on_recv_cb)`.
- `on_recv_cb`: cast data pointer to `espnow_payload_t*`; log node_id, msg_count, fake_temp.
- Blink LED once per received packet (non-blocking: use `xTaskNotify` to a blink task).

## Payload Struct

```c
typedef struct {
    uint8_t  node_id;
    uint16_t msg_count;
    float    fake_temp;
} __attribute__((packed)) espnow_payload_t;
```

Compile-time assert: `_Static_assert(sizeof(espnow_payload_t) <= 250, "Payload too large");`

## LED Feedback

- GPIO 21, active LOW.
- Sensor: 1 blink on send success; 3 fast blinks on failure.
- Gateway: 1 blink per received packet via notify task.
- `gpio_hold_dis(GPIO_NUM_21)` before driving (deep sleep may hold pad state on sensor node).

## Logging

- Tag: `"espnow"`
- Sensor wake: `ESP_LOGI(TAG, "Sensor #%d | msg #%u | sending to gateway", CONFIG_NODE_ID, msg_count)`
- Send result: `ESP_LOGI(TAG, "Send %s", status == ESP_NOW_SEND_SUCCESS ? "OK" : "FAIL")`
- Gateway recv: `ESP_LOGI(TAG, "RX node=%u msg=%u temp=%.1f", p->node_id, p->msg_count, p->fake_temp)`

## sdkconfig.defaults

```
CONFIG_ESP_CONSOLE_USB_CDC=y
CONFIG_NODE_ROLE_SENSOR=y
CONFIG_NODE_ID=1
CONFIG_GATEWAY_MAC="FF:FF:FF:FF:FF:FF"
```

(Operator changes `CONFIG_NODE_ROLE_GATEWAY=y` and `CONFIG_GATEWAY_MAC` on gateway board.)

## Error Handling

- `ESP_ERROR_CHECK` on all Wi-Fi and ESP-NOW init calls.
- `esp_now_send` return value: check and log but do not abort.
- Send timeout: log warning, skip LED blink, proceed to sleep.

## Agent-Generated Headers

- `.c` files: `/* AGENT-GENERATED — do not edit by hand; regenerate via esp32-sdd-full-project-generator */`
- CMakeLists.txt, sdkconfig.defaults, .gitignore, Kconfig.projbuild: `# AGENT-GENERATED — do not edit by hand; regenerate via esp32-sdd-full-project-generator`
- README.md: full HTML comment block.

## File Layout

- main/main.c
- main/CMakeLists.txt
- main/Kconfig.projbuild
- CMakeLists.txt
- sdkconfig.defaults
- .gitignore
- README.md
