# ==================== BOARD SELECTION ====================
# Change only this section, then regenerate the project
#
# Board: seeed-xiao-esp32s3          ← ACTIVE / DEFAULT
#
# =======================================================
# Institutional Memory: shared-specs/AIGenLessonsLearned.md — read before regenerating

# Coding Specification — Capacitive Touch Wakeup

## Touch Pad Configuration

- Touch channel: `TOUCH_PAD_NUM1` (maps to GPIO 1 / D0 on XIAO ESP32S3).
- `#define TOUCH_PAD   TOUCH_PAD_NUM1`
- `#define TOUCH_PAD_GPIO  GPIO_NUM_1`
- `#define CALIB_SAMPLES   50`
- `#define THRESHOLD_RATIO 0.70f`   // threshold = mean × 0.70

Use ESP-IDF Touch Sensor driver (`driver/touch_sensor.h`):
```c
touch_pad_init();
touch_pad_set_voltage(TOUCH_HVOLT_2V7, TOUCH_LVOLT_0V5, TOUCH_HVOLT_ATTEN_1V);
touch_pad_config(TOUCH_PAD, 0);   // threshold = 0 during calibration
touch_pad_filter_start(10);       // 10 ms filter period
```

## Calibration

On first boot (`ESP_SLEEP_WAKEUP_UNDEFINED`) and every timer wakeup:
```c
uint32_t sum = 0;
for (int i = 0; i < CALIB_SAMPLES; i++) {
    uint16_t val;
    touch_pad_read_filtered(TOUCH_PAD, &val);
    sum += val;
    vTaskDelay(pdMS_TO_TICKS(10));
}
uint32_t mean = sum / CALIB_SAMPLES;
uint32_t threshold = (uint32_t)(mean * THRESHOLD_RATIO);
touch_pad_set_thresh(TOUCH_PAD, threshold);
```

Store `threshold` in `RTC_DATA_ATTR uint32_t rtc_threshold` so it persists to touch wakeup.

## Deep Sleep Configuration

```c
touch_pad_set_thresh(TOUCH_PAD, rtc_threshold);   // restore from RTC memory
esp_sleep_enable_touchpad_wakeup();
esp_sleep_enable_timer_wakeup(20 * 1000000ULL);   // backup timer
esp_deep_sleep_start();
```

On touch wakeup, identify the triggered pad:
```c
touch_pad_t pad = esp_sleep_get_touchpad_wakeup_status();
```

## RTC Memory

```c
static RTC_DATA_ATTR uint32_t touch_count = 0;
static RTC_DATA_ATTR uint32_t rtc_threshold = 0;
```

- `touch_count`: increment only on `ESP_SLEEP_WAKEUP_TOUCHPAD`.
- `rtc_threshold`: set during calibration, read back on touch wakeup to log.

## LED Feedback

- GPIO 21, active LOW.
- Touch wakeup: 3 blinks — 200 ms LOW / 200 ms HIGH, repeated 3×.
- Timer wakeup: 1 slow blink — 500 ms LOW / 500 ms HIGH.
- `gpio_hold_dis(GPIO_NUM_21)` before driving; restore direction with `gpio_config_t`.

## Logging

- Tag: `"touch-wake"`
- On calibration: `ESP_LOGI(TAG, "Calibrated: mean=%lu threshold=%lu", mean, threshold)`
- On touch wake: `ESP_LOGI(TAG, "Touch wake! pad=%d  count=%lu  val≈%lu  thresh=%lu", pad, touch_count, read_val, rtc_threshold)`
- On timer wake: `ESP_LOGI(TAG, "Timer wake (no touch) — recalibrating")`
- Before sleep: `ESP_LOGI(TAG, "Sleeping (touch + 20 s timer)")`

## USB-CDC

- `CONFIG_ESP_CONSOLE_USB_CDC=y` in sdkconfig.defaults.
- `vTaskDelay(pdMS_TO_TICKS(100))` at start of `app_main` for CDC enumeration.

## Error Handling

- `ESP_ERROR_CHECK` on all `touch_pad_*` and sleep configuration calls.
- If `touch_pad_read_filtered` returns error during calibration: retry up to 3 times before aborting calibration and using a safe default threshold of 200.

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
