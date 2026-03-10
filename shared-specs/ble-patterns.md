# BLE Patterns

**Classification:** Human-authored shared spec
**Applies to:** All examples that include BLE advertising, beaconing, or manufacturer data payloads.

---

## Stack Selection

Choose the BLE stack based on the FunctionalSpec requirements:

| Criterion | NimBLE | Bluedroid |
|---|---|---|
| Beacon / deep-sleep use case | **Preferred** | Not recommended |
| Stack init time | ~50 ms | ~200 ms |
| RAM footprint | ~30 KB | ~60–80 KB |
| `nimble_port_stop()` teardown | Clean, synchronous | Not available |
| GATT server / client | Supported | Supported |
| Classic BT (A2DP, SPP, HFP) | Not supported | **Required** |
| BLE 5.0 extended advertising | Supported (ESP32-C3/S3/C6/H2) | Supported |

**Decision rule**: Use NimBLE for any beacon or beacon+sleep example. Use Bluedroid only when Classic BT or legacy GATT stacks are explicitly required by the FunctionalSpec.

---

## Non-Connectable Advertising — Parameter Template

```c
static struct ble_gap_adv_params adv_params = {
    .conn_mode = BLE_GAP_CONN_MODE_NON,   // non-connectable
    .disc_mode = BLE_GAP_DISC_MODE_NON,   // non-discoverable (beacon pattern)
    .itvl_min  = 160,  // 160 × 0.625 ms = 100 ms
    .itvl_max  = 160,
    .channel_map = 0,  // all three channels (37, 38, 39)
    .filter_policy = BLE_HCI_ADV_FILT_NONE,
    .high_duty_cycle = 0,
};
```

Advertising interval encoding: `value = desired_ms / 0.625`. Common values:
- 100 ms → 160
- 500 ms → 800
- 1000 ms → 1600

Source pattern: `bluedroid/ble/ble_ibeacon/main/ibeacon_demo.c`

---

## Advertising Payload Layout

The total advertising PDU is capped at **31 bytes**. Typical beacon layout:

| Offset | Length | Field | Value |
|---|---|---|---|
| 0 | 2 | Flags AD type + length | `0x02, 0x01` |
| 2 | 1 | Flags value | `0x06` (LE General Discoverable \| BR/EDR Not Supported) |
| 3 | 1 | Manufacturer data length | `N+2` (payload bytes + 2 for company ID) |
| 4 | 1 | Manufacturer data AD type | `0xFF` |
| 5 | 2 | Company ID (little-endian) | e.g., `0xFF, 0xFF` (test/internal) |
| 7 | N | Application payload | sensor data, sequence number, etc. |

Maximum application payload: `31 - 7 = 24 bytes` with the header above.

For iBeacon layout (Apple proprietary), prepend the iBeacon subtype `0x02, 0x15` after the company ID and follow with 16-byte UUID + 2-byte major + 2-byte minor + 1-byte TX power.

---

## BLE Stack Teardown Sequence

This exact call order is required before `esp_deep_sleep_start()`. Deviation leaves the radio powered.

```c
/* 1. Stop advertising (idempotent — safe to call even if already stopped) */
ble_gap_adv_stop();

/* 2. Signal the NimBLE host task to exit its event loop */
int rc = nimble_port_stop();
assert(rc == 0);

/* 3. Free NimBLE host resources */
nimble_port_deinit();

/* 4. Power down the BLE radio */
esp_err_t err = esp_bt_controller_disable();
ESP_ERROR_CHECK(err);

/* 5. Release controller memory back to heap */
err = esp_bt_controller_deinit();
ESP_ERROR_CHECK(err);
```

Reference: `nimble/power_save/main/main.c` in ESP-IDF examples.

**Why this order matters**: `nimble_port_stop()` posts an event to the NimBLE host task and blocks until the task exits. Calling `esp_bt_controller_disable()` before the host task exits causes an assertion in the controller.

---

## Power Save Integration

### Modem Sleep vs. Deep Sleep

| Mode | Use when | BLE teardown needed? |
|---|---|---|
| Modem sleep (`esp_wifi_set_ps` / BLE connection intervals) | BLE connection stays alive between events | No — controller manages radio duty cycle |
| Deep sleep | No BLE activity during sleep period | **Yes** — full teardown required |

### Typical Sleep Currents by Chip (deep sleep, RTC timer wakeup)

| Chip | Deep Sleep Current |
|---|---|
| ESP32 | ~10 µA |
| ESP32-S3 | ~8 µA |
| ESP32-C3 | ~5 µA |
| ESP32-C6 | ~7 µA (with LP core off) |
| ESP32-H2 | ~7 µA |

Values are approximate; consult the chip datasheet for the specific power mode and RTC peripheral configuration.

### Power Budget Formula

```
avg_current_µA = (active_mA × 1000 × active_window_ms + sleep_µA × sleep_period_ms)
                 / (active_window_ms + sleep_period_ms)
```

Include BLE teardown time (~5–10 ms) in `active_window_ms`.

---

## Advertising Window Timeout — Semaphore Pattern

NimBLE callbacks run on the NimBLE host task, not on `app_main`. Use a semaphore to synchronize:

```c
static SemaphoreHandle_t s_adv_done;
static esp_timer_handle_t s_adv_timer;

static void adv_timer_cb(void *arg)
{
    ble_gap_adv_stop();
    xSemaphoreGive(s_adv_done);
}

void app_main(void)
{
    s_adv_done = xSemaphoreCreateBinary();

    /* ... BLE init, set sync_cb, nimble_port_freertos_init() ... */

    esp_timer_create_args_t timer_args = {
        .callback = adv_timer_cb,
        .name = "adv_window",
    };
    esp_timer_create(&timer_args, &s_adv_timer);
    esp_timer_start_once(s_adv_timer, ADV_WINDOW_US); /* µs */

    /* Block until advertising window expires */
    xSemaphoreTake(s_adv_done, portMAX_DELAY);

    /* Now safe to tear down BLE stack */
    /* ... teardown sequence ... */

    esp_deep_sleep_start();
}
```

Source pattern: `ble_get_started/nimble/NimBLE_Beacon/` in ESP-IDF examples.

---

## sdkconfig Requirements — NimBLE Broadcaster-Only Role

Minimum Kconfig symbols for a NimBLE beacon (no scanning, no connections):

```
CONFIG_BT_ENABLED=y
CONFIG_BT_NIMBLE_ENABLED=y
CONFIG_BT_CONTROLLER_ENABLED=y
CONFIG_BT_NIMBLE_ROLE_BROADCASTER=y
CONFIG_BT_NIMBLE_ROLE_OBSERVER=n
CONFIG_BT_NIMBLE_ROLE_CENTRAL=n
CONFIG_BT_NIMBLE_ROLE_PERIPHERAL=n
CONFIG_BT_NIMBLE_MAX_CONNECTIONS=0
```

Disabling observer/central/peripheral roles reduces NimBLE RAM footprint by ~4–8 KB.

For BLE 5.0 extended advertising (ESP32-C3/S3/C6/H2 only), additionally set:
```
CONFIG_BT_NIMBLE_EXT_ADV=y
```

---

## Common Gotchas

| Issue | Root Cause | Fix |
|---|---|---|
| `assert` crash in `esp_bt_controller_disable()` | Called before NimBLE host task exits | Always call `nimble_port_stop()` and wait for it to return before disabling controller |
| Advertising never stops | Timer callback calls `ble_gap_adv_stop()` from wrong context | `esp_timer` callbacks are safe; FreeRTOS timer callbacks are also safe; ISR callbacks are not |
| 31-byte PDU overflow | Payload + headers exceed limit | Count bytes: Flags (3) + Mfr header (4) + company ID (2) = 9 bytes consumed before payload |
| GPIO state lost after wakeup | `gpio_hold_en()` not called before deep sleep | Call `gpio_hold_en(pin)` after BLE teardown, before `esp_deep_sleep_start()` |
| High average current | BLE teardown skipped — radio stays powered during sleep | Always execute full teardown sequence before `esp_deep_sleep_start()` |
| NimBLE host task stack overflow | Default stack size too small for complex callbacks | Set `CONFIG_BT_NIMBLE_HOST_TASK_STACK_SIZE` to at least 4096 |
| `undefined reference to 'ble_sm_deinit'` linker error on broadcaster-only builds | ESP-IDF 5.5.x bug: `ble_hs.c` calls `ble_sm_deinit()` when `NIMBLE_BLE_SM=1`, but `ble_sm.c` only defines it when `BLE_STATIC_TO_DYNAMIC=1` (default: 0) | Set `CONFIG_BT_NIMBLE_SECURITY_ENABLE=n` — a non-connectable broadcaster has no use for the SM, and disabling it makes `NIMBLE_BLE_SM=0` |
| `sdkconfig.defaults` changes not applied after menuconfig | ESP-IDF applies `sdkconfig.defaults` only when `sdkconfig` does not exist; an existing `sdkconfig` always wins | Delete `sdkconfig` and `sdkconfig.old` at the project root before a clean rebuild |
| Boot button not usable for deep-sleep wakeup on ESP32-C6/C5 | GPIO 9 (XIAO boot button on C6 and C5) is not an RTC GPIO; only GPIO 0–7 (C6) or 0–6 (C5) support `ext1` wakeup. `esp_sleep_enable_ext1_wakeup()` will return `ESP_ERR_INVALID_ARG` or silently fail to wake at runtime. | Use timer-only wakeup on these targets. If button wakeup is essential, connect an external button to an LP-capable GPIO (0–7 on C6, 0–6 on C5). Document the limitation in the Supported Boards table. |
