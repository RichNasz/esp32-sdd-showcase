# ==================== BOARD SELECTION ====================
# Change only this section, then regenerate the project
#
# Board: seeed-xiao-esp32s3          ← ACTIVE / DEFAULT
#
# =======================================================
# Institutional Memory: shared-specs/AIGenLessonsLearned.md — read before regenerating

# Coding Specification — BLE Beacon + Deep Sleep

## Architecture

Single-file firmware. App_main runs once per wake cycle: increment RTC counters, broadcast a
non-connectable BLE advertisement for 100 ms using NimBLE, tear down the BLE stack completely,
then enter deep sleep. The advertising-complete callback drives the teardown — app_main waits
on a synchronisation primitive rather than polling. Both a 20-second timer wakeup and a GPIO 0
ext0 wakeup are configured so the user can trigger an on-demand beacon burst.

## Key Constraints

- Sleep duration: 20 seconds.
- Advertising window: exactly 100 ms. The BLE stack must be fully initialized and torn down
  within this window before entering deep sleep.
- Persistence: both boot_count and sequence are RTC_DATA_ATTR and increment on every wake cycle.
- Manufacturer data payload: includes a fixed company ID, boot_count, sequence, and a 4-byte
  magic identifier. The total ADV PDU must fit within the 31-byte BLE advertising limit.
- Ext0 wakeup on GPIO 0 (active LOW) provides on-demand wakeup in addition to the timer.

## Preferred Libraries and APIs

Use the NimBLE stack, not Bluedroid. NimBLE is significantly lighter and faster to initialise
and tear down, which matters for a 100 ms advertising window. Bluedroid requires substantially
more RAM and initialization time, making it poorly suited to duty-cycled beacon applications.

Use esp_sleep for wakeup source configuration and esp_sleep_get_wakeup_cause() to distinguish
timer from button wakeup.

## Non-Functional Requirements

- The BLE stack must be completely torn down — including controller deinitialization and memory
  release — before entering deep sleep. Leaving the controller running drains power and can
  cause instability on the next wake cycle.
- If advertising fails to start, the device must log the error and proceed directly to deep
  sleep. It must never block indefinitely waiting for BLE resources.
- LED on GPIO 21 is active LOW. gpio_hold_dis() must be called before driving the pin on any
  wake from deep sleep.

## Gotchas

- NimBLE requires CONFIG_BT_ENABLED, CONFIG_BT_NIMBLE_ENABLED, and CONFIG_BT_CONTROLLER_ENABLED
  in sdkconfig. Missing any one of these causes a link-time error that is difficult to diagnose
  from the error message alone.
- The advertising-complete callback fires on the NimBLE task stack. Synchronisation with
  app_main must use a semaphore or event group — do not attempt to call blocking sleep APIs
  from within the callback.
- esp_bt_controller_deinit() must be called before entering deep sleep. Failure to do so leaves
  the BLE controller powered, with a significant impact on the device's power budget.

## File Layout (non-standard files)

None beyond the standard layout.
