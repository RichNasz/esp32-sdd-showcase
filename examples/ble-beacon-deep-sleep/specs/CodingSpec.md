# ==================== BOARD SELECTION ====================
# This project supports four boards, selected via `idf.py set-target`:
#
#   esp32s3  →  Seeed XIAO ESP32S3  (LED GPIO 21 active-LOW, GPIO 0 RTC wakeup)
#   esp32c6  →  Seeed XIAO ESP32-C6 (LED GPIO 15 active-LOW, timer-only wakeup)
#   esp32c5  →  Seeed XIAO ESP32-C5 (LED GPIO 27 active-LOW, timer-only wakeup)
#   esp32    →  Adafruit HUZZAH32   (LED GPIO 13 active-HIGH, GPIO 0 RTC wakeup)
#
# Board-specific GPIO is set in sdkconfig.defaults.<target> via Kconfig.
# =======================================================
# Institutional Memory: shared-specs/AIGenLessonsLearned.md — read before regenerating

# Coding Specification — BLE Beacon + Deep Sleep

## Architecture

Single-file firmware. App_main runs once per wake cycle: increment RTC counters, broadcast a
non-connectable BLE advertisement for 10 s using NimBLE, tear down the BLE stack completely,
then enter deep sleep. The LED is lit while the beacon is broadcasting and turned off before
entering deep sleep. The advertising-complete callback drives the teardown — app_main waits
on a synchronisation primitive rather than polling.

Wakeup strategy is target-dependent:
- **ESP32-S3 and ESP32**: 10-second timer wakeup + GPIO button wakeup (ext1 on S3, ext0 on ESP32).
  The boot button GPIO is an RTC-capable GPIO on these targets.
- **ESP32-C6 and ESP32-C5**: 10-second timer wakeup only. The boot button (GPIO 9) is not an
  RTC-capable GPIO on C6 (RTC range: 0–7) or C5 (RTC range: 0–6), so ext wakeup is skipped.

## Key Constraints

- Sleep duration: 10 seconds.
- Advertising window: 10 seconds. The BLE stack must be fully initialized and torn down
  after this window before entering deep sleep.
- Persistence: both boot_count and sequence are RTC_DATA_ATTR and increment on every wake cycle.
- Advertising payload must include:
  1. Complete Local Name AD element: the fixed string `"ESP32-SDD"`. This makes the beacon
     identifiable by name in BLE scanner apps (e.g. nRF Connect) without MAC filtering.
  2. Manufacturer data AD element: fixed company ID 0xFFFF, boot_count (2 bytes LE), sequence
     (2 bytes LE), and 4-byte magic 0xDEADBEEF.
  Total ADV PDU must remain ≤ 31 bytes. Current budget: Flags (3) + Mfr data (12) + Local Name (10) = 25 bytes.
- After NimBLE host sync fires, read the device BLE MAC address and log it as
  `BLE MAC: XX:XX:XX:XX:XX:XX` before starting advertising. Use `ble_hs_id_copy_addr()` or
  equivalent NimBLE API. This lets the user find the specific device in a scanner app.

## Preferred Libraries and APIs

Use the NimBLE stack, not Bluedroid. NimBLE is significantly lighter and faster to initialise
and tear down, which matters for a duty-cycled beacon with a 10 s advertising window. Bluedroid requires substantially
more RAM and initialization time, making it poorly suited to duty-cycled beacon applications.

Use esp_sleep for wakeup source configuration and esp_sleep_get_wakeup_cause() to distinguish
timer from button wakeup.

## Non-Functional Requirements

- The BLE stack must be completely torn down before entering deep sleep. The
  `esp32-ble-beacon-engineer` skill owns the exact teardown sequence. Leaving the radio
  running drains power and can cause instability on the next wake cycle.
- If advertising fails to start, the device must log the error and proceed directly to deep
  sleep. It must never block indefinitely waiting for BLE resources.
- The user LED must be turned on when advertising starts and turned off after advertising stops,
  before entering deep sleep. This gives the user a visible indication that the beacon is active.
  LED GPIO and active level are board-specific (see Kconfig symbols CONFIG_EXAMPLE_LED_GPIO and
  CONFIG_EXAMPLE_LED_ACTIVE_LEVEL). gpio_hold_dis() must be called before driving the LED pin
  on any wake from deep sleep; gpio_hold_en() must be called after LED off, before sleep.
- Button wakeup GPIO (CONFIG_EXAMPLE_BUTTON_GPIO) is only defined for targets where the boot
  button is RTC-capable (ESP32-S3, ESP32). The Kconfig symbol must depend on the target to
  avoid misuse on C6/C5.

## Gotchas

- NimBLE requires CONFIG_BT_ENABLED, CONFIG_BT_NIMBLE_ENABLED, CONFIG_BT_CONTROLLER_ENABLED,
  and CONFIG_BT_NIMBLE_SECURITY_ENABLE=n in sdkconfig. Missing the first three causes a
  link-time error; missing the last causes `undefined reference to 'ble_sm_deinit'` on
  broadcaster-only builds (ESP-IDF 5.5.x bug).
- The advertising-complete callback fires on the NimBLE task stack. Synchronisation with
  app_main must use a semaphore or event group — do not attempt to call blocking sleep APIs
  from within the callback.
- The full BLE teardown sequence (see `esp32-ble-beacon-engineer` skill step 6) must complete
  before entering deep sleep. Failure to tear down leaves the radio powered, with a significant
  impact on the device's power budget.

## File Layout (non-standard files)

None beyond the standard layout.

## Monitor Dashboard

### Pattern choice: per-cycle snapshot

This firmware is duty-cycled (10 s active + 10 s sleep). The correct ANSI pattern is
**per-cycle snapshot** (cursor-home + overwrite). A scroll region (Pattern A from the
`esp32-ansi-monitor-engineer` skill) is not appropriate: there is no continuous event stream to
scroll, and the fixed-row layout rewrites itself cleanly on each 250 ms tick. The screen freezes
during deep sleep — this is expected and provides a visible cue that the chip has entered sleep.

### Dashboard layout

Six fixed rows, redrawn every ~250 ms during the 10 s advertising window:

```
Row 1: title bar — "BLE BEACON MONITOR  ESP32-SDD" (bold, coloured background)
Row 2: Cycle: #N    Wakeup: TIMER / BUTTON / FIRST_BOOT
Row 3: BLE MAC: XX:XX:XX:XX:XX:XX  (shown as "—" until ble_sync_cb fires)
Row 4: Status:  INITIALIZING / ADVERTISING / SLEEPING
Row 5: Active:  [═══════════     ] 7.2 s remaining  (text progress bar, 20 chars wide)
Row 6: separator line
```

Status field state machine:
- `INITIALIZING` — from app_main entry until `ble_sync_cb` fires
- `ADVERTISING` — from `ble_sync_cb` until `adv_timer_cb` fires and advertising stops
- `SLEEPING` — drawn once after BLE teardown; chip enters deep sleep immediately after

Progress bar in row 5: proportional to elapsed time in the 10 s advertising window.
Computed from the difference between current `esp_timer_get_time()` and a start timestamp
recorded when advertising begins. Freezes at full when status becomes `SLEEPING`.

### Refresh architecture

The main task must wake every ~250 ms to redraw during the advertising window. Two signals
share the same wakeup path:
- A new periodic 250 ms `esp_timer` posts a task notification each tick.
- The existing `adv_timer_cb` (10 s advertising window expiry) additionally posts a task
  notification and sets an atomic flag indicating advertising is complete.
- The main task loops on `xTaskNotifyWait()` with a short timeout, redraws on each wakeup,
  and exits the loop when the advertising-complete flag is set.

Only the `app_main` task writes to the terminal. Timer callbacks and NimBLE callbacks must
only set flags or call `xTaskNotifyGive()` — never call `esp_rom_printf` directly.

### Output discipline

All terminal output after the initial screen clear must use `esp_rom_printf()` exclusively
(ROM UART, zero buffering; bypasses newlib stdio and the VFS layer entirely). Pre-format
strings with `snprintf` into a local stack buffer, then output with `esp_rom_printf("%s", buf)`.
Do not pass format specifiers directly to `esp_rom_printf` — ROM printf width and precision
support is not guaranteed across all ESP-IDF builds.

Call `esp_log_level_set("*", ESP_LOG_NONE)` immediately after clearing the screen and drawing
the first frame. After that point, no ESP-IDF log output must appear. Errors must be reflected
in the `Status` field of the dashboard rather than via `ESP_LOGE` calls.

Include `esp_rom_sys.h` for `esp_rom_printf`. If the linker reports an undefined reference, add
`esp_rom` explicitly to REQUIRES in `main/CMakeLists.txt`.

### USB-native board note

Boards with a native USB peripheral (XIAO ESP32S3, XIAO ESP32-C6, XIAO ESP32-C5) lose the
serial port when the chip enters deep sleep. The dashboard will freeze on the `SLEEPING` frame
and the terminal disconnects. This is expected behavior; the `SLEEPING` status and full progress
bar are a deliberate last-frame indicator before the port disappears. Direct users in README and
TestSpec to the Adafruit HUZZAH32 (CP2104 UART bridge) for multi-cycle serial observation —
it keeps the port alive across sleep cycles.
