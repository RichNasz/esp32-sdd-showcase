# Coding Conventions

**Classification:** Human-authored shared spec
**Applies to:** All C source generated for examples in this repository

---

## Language and Standard

- **Language:** C17 (`-std=gnu17`; GNU extensions are acceptable for ESP-IDF compatibility).
- **C++ is not used** unless a third-party component requires it. All application code is C.
- No dynamic allocation after initialization unless explicitly justified in the `CodingSpec.md`.

---

## Naming Conventions

| Element | Convention | Example |
|---|---|---|
| Global functions | `snake_case`, prefixed with the component name | `sensor_read_temperature()` |
| Static (file-scope) functions | `snake_case`, no prefix required | `apply_median_filter()` |
| Global variables | `g_snake_case` | `g_sensor_handle` |
| Constants / macros | `SCREAMING_SNAKE_CASE` | `SENSOR_READ_INTERVAL_MS` |
| Typedefs for structs | `snake_case_t` | `sensor_config_t` |
| Enum values | `PREFIX_VALUE` | `SENSOR_STATE_IDLE` |
| FreeRTOS task functions | `snake_case_task` | `sensor_poll_task` |
| FreeRTOS queue/semaphore handles | `snake_case_queue` / `snake_case_sem` | `adc_data_queue` |
| Kconfig symbols | `CONFIG_COMPONENT_SYMBOL` | `CONFIG_SENSOR_POLL_RATE_HZ` |

---

## Error Handling

- All ESP-IDF API calls that return `esp_err_t` must be checked. Use `ESP_ERROR_CHECK()` during initialization. In runtime loops, check and log without aborting unless the error is unrecoverable.
- Log using the `esp_log` macros (`ESP_LOGI`, `ESP_LOGW`, `ESP_LOGE`). Every component defines its own `TAG` as a `static const char * TAG = "component_name";`.
- On unrecoverable errors, call `esp_restart()` after logging — do not use infinite loops that silently hang.
- Never suppress `esp_err_t` return values by casting to `void` in production paths. Initialization paths use `ESP_ERROR_CHECK()`; runtime paths check and decide explicitly.

---

## Memory Management

- Prefer stack allocation. Heap allocation is acceptable for buffers whose size is not known at compile time, for handles, and for FreeRTOS primitives.
- All heap allocations must check the returned pointer for NULL before use. Log and restart on NULL from critical allocations.
- Use `heap_caps_malloc()` with `MALLOC_CAP_DMA` or `MALLOC_CAP_SPIRAM` where DMA or PSRAM is required. Do not use plain `malloc()` for DMA buffers.
- Free in reverse allocation order. No memory leaks are acceptable in the steady-state loop.

---

## FreeRTOS Usage

- Task stack sizes must be specified in the `CodingSpec.md` and set via Kconfig (`CONFIG_COMPONENT_TASK_STACK_SIZE`). Do not hardcode stack sizes in source.
- Task priorities must be declared in the `CodingSpec.md`. Application tasks run at priorities 1–5; system tasks (Wi-Fi, BT, lwIP) run at higher priorities — never block them.
- All inter-task communication must use FreeRTOS primitives (queues, semaphores, event groups). No global shared state without a mutex.
- Always specify a finite timeout on `xQueueReceive`, `xSemaphoreTake`, etc. Infinite waits (`portMAX_DELAY`) are only acceptable in tasks whose sole job is to wait on that primitive.
- Never call blocking FreeRTOS APIs from ISR context. Use `xQueueSendFromISR` / `xSemaphoreGiveFromISR` and yield if needed.

---

## ESP-IDF Patterns

- Initialize the default event loop once: `esp_event_loop_create_default()`. Register component event handlers with `esp_event_handler_register()`.
- NVS: always call `nvs_flash_init()` before any NVS usage. Handle `ESP_ERR_NVS_NO_FREE_PAGES` and `ESP_ERR_NVS_NEW_VERSION_FOUND` by erasing and reinitializing.
- GPIO ISR service: install once with `gpio_install_isr_service(0)` before any `gpio_isr_handler_add()` calls.
- Timers: prefer `esp_timer` (high-resolution, runs in task context when using `ESP_TIMER_TASK`) over FreeRTOS timers for one-shot and periodic use cases.
- UART: always configure baud, data bits, stop bits, and parity explicitly in `uart_config_t`. Do not rely on defaults.
- I2C / SPI: use the `i2c_master` / `spi_master` driver APIs from ESP-IDF v5. The legacy `i2c` driver is deprecated; do not use it.

---

## Code Layout

- One component per logical subsystem. Components live in `components/<name>/` with their own `CMakeLists.txt`.
- Each component exposes a single public header: `components/<name>/include/<name>.h`.
- Implementation files are in `components/<name>/src/<name>.c` (or multiple `.c` files if warranted).
- `main/` contains only the application entry point (`app_main`) and top-level wiring. Business logic lives in components.
- Maximum file length: 500 lines. Split into multiple files if exceeded.
- Maximum function length: 80 lines. Extract sub-functions if exceeded.

---

## Watchdog

- Enable the task watchdog (`CONFIG_ESP_TASK_WDT_EN=y`) in all examples.
- Subscribe application tasks to the watchdog with `esp_task_wdt_add(NULL)` and call `esp_task_wdt_reset()` at the top of every steady-state loop iteration.
- Deep-sleep examples must call `esp_task_wdt_delete(NULL)` before entering sleep.

---

## Logging

- Use log levels appropriately: `ESP_LOGD` for verbose debug, `ESP_LOGI` for normal operational events, `ESP_LOGW` for recoverable anomalies, `ESP_LOGE` for errors.
- Set the default log level via `CONFIG_LOG_DEFAULT_LEVEL` in `sdkconfig.defaults`. Debug builds use `DEBUG`; release builds use `INFO` or `WARN`.
- Do not leave `ESP_LOGD` calls in hot paths (ISRs, tight loops) in production.

---

## Comments and Documentation

- Comments explain **why**, not **what**. Code is self-documenting through good naming.
- Every public function in a component header must have a Doxygen-style block comment: `@brief`, `@param`, `@return`.
- Non-obvious algorithms or hardware workarounds must include a comment referencing the datasheet section or errata note.
- Generated files carry the standard agent-generated header. Human-authored files carry no header (or a human-authored marker as in `AIGenLessonsLearned.md`).
