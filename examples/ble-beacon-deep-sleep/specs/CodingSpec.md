# ==================== BOARD SELECTION ====================
# Change only this section, then regenerate the project
#
# Board: seeed-xiao-esp32s3          ← ACTIVE / DEFAULT
#
# =======================================================
# Institutional Memory: shared-specs/AIGenLessonsLearned.md — read before regenerating

# Coding Specification — BLE Beacon + Deep Sleep

## Deep Sleep Configuration

- Timer wakeup: `esp_sleep_enable_timer_wakeup(20 * 1000000ULL)`.
- Ext0 wakeup: `esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0)` (GPIO 0, active LOW).
- Enter sleep: `esp_deep_sleep_start()` after advertising stops.
- `#define SLEEP_SEC 20`
- `#define ADV_WINDOW_MS 100`

## RTC Memory

- `static RTC_DATA_ATTR uint16_t boot_count = 0;`
- `static RTC_DATA_ATTR uint16_t sequence = 0;`
- Increment both at the top of `app_main`.

## BLE Stack

- Use ESP-IDF NimBLE stack (lighter than Bluedroid for beacon-only use).
- Enable via sdkconfig.defaults: `CONFIG_BT_NIMBLE_ENABLED=y`, `CONFIG_BT_CONTROLLER_ENABLED=y`.
- Init: `esp_nimble_hci_init()` → `nimble_port_init()`.
- Advertising type: `BLE_GAP_CONN_MODE_NON` (non-connectable), `BLE_GAP_DISC_MODE_NON` (non-discoverable).
- No scan response packet.
- Advertising interval: `ADV_WINDOW_MS` × 1000 / 625 µs units = 160 units (100 ms).
- Set duration: advertise for exactly `ADV_WINDOW_MS` ms then stop.

## Advertisement Payload

Build using `ble_gap_adv_params_t` and `os_mbuf`:

```
Flags: 0x02 (General Discoverable | BR/EDR Not Supported)
Manufacturer Specific Data (type 0xFF):
  Company ID:  0xFF 0xFF  (little-endian)
  boot_count:  2 bytes LE
  sequence:    2 bytes LE
  Magic:       0xDE 0xAD 0xBE 0xEF
```

Total manufacturer data: 10 bytes. Total ADV PDU ≤ 31 bytes — fits without truncation.

## Advertising Lifecycle

1. `ble_gap_adv_start()` with `duration_ms = ADV_WINDOW_MS`.
2. Register `ble_gap_event_fn` callback; on `BLE_GAP_EVENT_ADV_COMPLETE`: call `nimble_port_stop()`.
3. After NimBLE port stops: deinit BLE controller (`esp_bt_controller_deinit()`), release memory.
4. `esp_deep_sleep_start()`.

## LED Feedback

- GPIO 21, active LOW.
- Drive LOW at start of advertising, HIGH when advertising stops.
- `gpio_hold_dis(GPIO_NUM_21)` before driving.

## Logging

- Tag: `"ble-beacon"`
- On wake: log boot_count, sequence, wakeup cause.
- On adv start: `ESP_LOGI(TAG, "Advertising — boot=%u seq=%u", boot_count, sequence)`
- On adv stop: `ESP_LOGI(TAG, "Done — sleeping %d s", SLEEP_SEC)`

## sdkconfig.defaults

```
CONFIG_BT_ENABLED=y
CONFIG_BT_NIMBLE_ENABLED=y
CONFIG_BT_CONTROLLER_ENABLED=y
CONFIG_ESP_CONSOLE_USB_CDC=y
```

## Error Handling

- `ESP_ERROR_CHECK` on all BLE init and sleep configuration calls.
- If `ble_gap_adv_start()` returns non-zero: log error and proceed to sleep (never block).

## Agent-Generated Headers

- `.c` files: `/* AGENT-GENERATED — do not edit by hand; regenerate via esp32-sdd-full-project-generator */`
- CMakeLists.txt, sdkconfig.defaults, .gitignore: `# AGENT-GENERATED — do not edit by hand; regenerate via esp32-sdd-full-project-generator`
- README.md: full HTML comment block.

## File Layout

- main/main.c
- main/CMakeLists.txt
- CMakeLists.txt
- sdkconfig.defaults
- .gitignore
- README.md
