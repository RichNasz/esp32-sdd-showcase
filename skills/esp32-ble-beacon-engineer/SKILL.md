---
name: esp32-ble-beacon-engineer
description: Designs and generates BLE advertising/beaconing configurations for ESP32. Handles NimBLE stack init/teardown, non-connectable advertising, manufacturer data PDU construction, bounded advertising windows, and BLE stack teardown before deep sleep.
version: "1.0.0"
author: Richard Naszcyniec
keywords: sdd, esp32, ble, nimble, beacon, advertising, ibeacon, manufacturer-data, deep-sleep, power-budget
---

<!-- ================================================
     AGENT-GENERATED — DO NOT EDIT BY HAND
     Generated from specs/ using esp32-sdd-documentation-generator skill
     Date: 2026-03-10 | Agent: Claude Code
     ================================================ -->

# ESP32 BLE Beacon Engineer

## When to use
Any time a FunctionalSpec requires BLE advertising, beaconing, manufacturer data payloads, or iBeacon/Eddystone frames. Invoked automatically as a sub-workflow by `esp32-sdd-full-project-generator` when BLE advertising or beaconing is specified. Can also be run standalone.

## Workflow (follow exactly)

1. **Read FunctionalSpec BLE requirements**: Extract advertising type (connectable vs. non-connectable), payload format (manufacturer data, iBeacon, Eddystone), advertising interval, advertising window duration, and target chip. Note whether deep sleep follows the advertising window.

2. **Select BLE stack**: Choose **NimBLE** for beacon/deep-sleep patterns — it has a lighter init/teardown path, faster startup time (~50 ms vs ~200 ms for Bluedroid), and a cleaner `nimble_port_stop()` sequence. Choose **Bluedroid** only if GATT services or Classic BT (A2DP, SPP, HFP) are also required by the FunctionalSpec. Document the rationale in a comment at the top of `main/ble_beacon.c`.

3. **Generate sdkconfig.defaults fragment** for the BLE stack selection:
   - For NimBLE (beacon/sleep use case):
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
   - For Bluedroid (GATT/Classic use case): include `CONFIG_BT_BLUEDROID_ENABLED=y` and disable NimBLE keys. Consult `shared-specs/ble-patterns.md` for the full symbol list.

4. **Generate advertising init/start sequence** in `main/ble_beacon.c`:
   - Call order: `esp_bt_controller_init()` → `esp_bt_controller_enable(ESP_BT_MODE_BLE)` → `nimble_port_init()` → set `ble_hs_cfg.sync_cb` → `nimble_port_freertos_init(ble_host_task)`
   - In the sync callback, configure advertising parameters:
     - `conn_mode = BLE_GAP_CONN_MODE_NON` (non-connectable)
     - `disc_mode = BLE_GAP_DISC_MODE_NON` (non-discoverable) unless the spec requires discoverability
     - `itvl_min` / `itvl_max` from FunctionalSpec (units: 0.625 ms; e.g., 160 = 100 ms)
   - Build the manufacturer data PDU: total advertising payload must not exceed 31 bytes. Layout: `FF <company_id_lo> <company_id_hi> <payload...>`. Reference `shared-specs/ble-patterns.md` for the byte map.
   - Start advertising via `ble_gap_adv_start()`.

5. **Generate bounded advertising window** using a semaphore or event group:
   - Create a `SemaphoreHandle_t adv_complete_sem` before starting the NimBLE host task.
   - Create an `esp_timer` for the advertising window duration specified in the FunctionalSpec.
   - On timer expiry (timer callback runs in timer task context): call `ble_gap_adv_stop()` then `xSemaphoreGive(adv_complete_sem)`.
   - In `app_main`: after `nimble_port_freertos_init()`, block on `xSemaphoreTake(adv_complete_sem, portMAX_DELAY)`.
   - **Never** use `vTaskDelay` alone as the advertising timeout — NimBLE callbacks run on the NimBLE host task, not on `app_main`, so `vTaskDelay` in `app_main` does not block advertising correctly.

6. **Generate complete BLE stack teardown sequence** (mandatory before deep sleep):
   Execute in this exact order after the semaphore is taken:
   ```c
   ble_gap_adv_stop();          // idempotent — safe to call again
   nimble_port_stop();          // signals host task to exit
   nimble_port_deinit();        // frees NimBLE resources
   esp_bt_controller_disable(); // powers down radio
   esp_bt_controller_deinit();  // releases controller memory
   ```
   Skipping any step leaves the BLE radio powered during deep sleep, destroying the power budget. Reference: `nimble/power_save/main/main.c` in ESP-IDF examples.

7. **Coordinate with deep sleep** (if FunctionalSpec specifies a wakeup strategy):
   - Teardown sequence (step 6) must complete before `esp_deep_sleep_start()` is called.
   - The `esp32-deep-sleep-engineer` sub-workflow owns the `esp_deep_sleep_start()` call and wakeup source configuration — do not duplicate that logic here.
   - Add a note to the README power budget table that the active window includes BLE teardown time (~5–10 ms).
   - If `gpio_hold_en()` is used for peripheral power gating (from the deep-sleep engineer output), ensure it is called after BLE teardown and before `esp_deep_sleep_start()`.

8. **Document in the example README**:
   - NimBLE vs. Bluedroid choice rationale (one sentence).
   - Advertising payload byte map (table: offset, length, value, description).
   - Power budget breakdown: sleep current (µA) + active window current (mA) × duration (ms) = average µA.
   - Reference to `shared-specs/ble-patterns.md` for extended pattern documentation.
