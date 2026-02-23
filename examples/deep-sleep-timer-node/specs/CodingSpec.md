# ==================== BOARD SELECTION ====================
# Change only this section, then regenerate the project
#
# Board: seeed-xiao-esp32s3          ← ACTIVE / DEFAULT
#
# =======================================================
# Institutional Memory: shared-specs/AIGenLessonsLearned.md — read before regenerating

# Coding Specification — Deep Sleep Timer Node

## Deep Sleep Configuration

- Wakeup source: `esp_sleep_enable_timer_wakeup(SLEEP_DURATION_US)`.
- `#define SLEEP_SEC       15`
- `#define SLEEP_DURATION_US  ((uint64_t)SLEEP_SEC * 1000000ULL)`
- Enter sleep: `esp_deep_sleep_start()` — no return.
- On wake: `esp_sleep_get_wakeup_cause()` → branch on `ESP_SLEEP_WAKEUP_TIMER` vs `ESP_SLEEP_WAKEUP_UNDEFINED` (first boot / power-on reset).

## RTC Memory — Boot Counter

- `static RTC_DATA_ATTR uint32_t boot_count = 0;`
- Increment `boot_count` as the **first statement** in `app_main`, before logging.
- RTC_DATA_ATTR survives deep sleep but resets on power-cycle or ULP wakeup power-off.
- No NVS required; this example demonstrates the simplest persistence mechanism.

## LED Feedback

- LED GPIO: 21 (active LOW, per board-spec).
- On each wake: `gpio_hold_dis(GPIO_NUM_21)` → drive LOW for 100 ms → drive HIGH.
- Use `gpio_config_t` with `GPIO_MODE_OUTPUT` / `GPIO_PULLUP_ENABLE`.
- Do NOT use LEDC; simple `gpio_set_level` is sufficient.
- Hold not required before sleep (LED stays HIGH = off, no need to hold).

## Logging

- Tag: `"sleep-node"`
- First boot: `ESP_LOGI(TAG, "First boot — counter initialized")`
- Timer wake: `ESP_LOGI(TAG, "Wake #%lu | TIMER wakeup | sleeping for %d s", (unsigned long)boot_count, SLEEP_SEC)`
- Unknown cause: `ESP_LOGW(TAG, "Wake #%lu | unknown wakeup cause (%d)", (unsigned long)boot_count, cause)`

## Timing Budget

- LED blink: 100 ms
- Logging + serial flush: ~50 ms
- ESP-IDF startup overhead: ~50 ms
- Total active window target: ≤ 200 ms

## USB-CDC (XIAO ESP32S3)

- `CONFIG_ESP_CONSOLE_USB_CDC=y` in sdkconfig.defaults.
- Add `vTaskDelay(pdMS_TO_TICKS(100))` after `app_main` entry to allow USB-CDC to enumerate before first log line.

## Error Handling

- Wrap every ESP-IDF call with `ESP_ERROR_CHECK`.
- If wakeup cause is unrecognised: log WARNING and proceed to sleep (never abort in production deep-sleep loop).

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
