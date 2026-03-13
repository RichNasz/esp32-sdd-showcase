<!-- ================================================
     HUMAN-WRITTEN — GOLD STANDARD INSTITUTIONAL MEMORY
     ================================================ -->

# AIGenLessonsLearned.md — Gold Standard Lessons Learned

**MANDATORY FIRST STEP**: Every time you generate, regenerate, fix, or debug ANY code in this repository, read this file first.

## Core Rules (Never Violate)
- Humans only ever edit files inside any `specs/` folder.
- Never edit generated files (main/, CMakeLists.txt, sdkconfig.defaults, README.md, etc.) by hand.
- If something is wrong, improve the spec — never patch the generated code.
- Always respect the permanent example structure in specs/example-project-structure-spec.md
- Always reference board-specs/ files instead of hard-coding pins.

## Key Lessons Learned from Real AI Generation (Updated Mar 2026)
- Precise board references + board-specs/*.md dramatically reduce hallucinations.
- Explicit power budgets, wakeup sources, and peripheral constraints in FunctionalSpec.md are essential for ESP32.
- When switching boards (e.g. HUZZAH32 ↔ XIAO ESP32S3), change only the one-line board selector and regenerate.
- Always specify FreeRTOS task priorities, stack sizes, and queue usage explicitly.
- Include watchdog, error handling, and deep-sleep current targets in every spec.
- Sine-wave breathing, debouncing, and state machines work best when the algorithm is fully described in the CodingSpec.
- The first error is almost always a missing constraint in the spec, not a bad AI.
- **Kconfig.projbuild must live in a component directory** (e.g. `main/`), not the project
  root. ESP-IDF only auto-discovers Kconfig.projbuild inside component dirs. Placing it at
  the root silently drops all its symbols, causing #error guards and undeclared-identifier
  cascades at compile time. (Feb 2026)
- **CMakeLists.txt ordering: `include(project.cmake)` BEFORE `project()`**. ESP-IDF injects
  the cross-compiler toolchain via project.cmake, which must be loaded before CMake's
  `project()` call initialises the compiler. Reversing the order causes CMake to detect the
  host system compiler (Apple Clang on macOS) instead of the RISC-V/Xtensa cross-compiler,
  silently breaking the build. The cmake-conventions.md spec has this wrong — always use:
  `include($ENV{IDF_PATH}/tools/cmake/project.cmake)` then `project(name)`. (Mar 2026)
- **Stack overflow Kconfig symbol is `FREERTOS_CHECK_STACKOVERFLOW_CANARY`**, not
  `ESP_SYSTEM_CHECK_STACKOVERFLOW_CANARY`. The latter does not exist in ESP-IDF 5.x and
  produces an "unknown kconfig symbol" warning that silently drops the safety setting.
  Use `CONFIG_FREERTOS_CHECK_STACKOVERFLOW_CANARY=y` in sdkconfig.defaults. (Mar 2026)

- **ESP32-S3 does not support `esp_sleep_enable_ext0_wakeup()`**. ext0 is an ESP32-original-only API.
  On ESP32-S3 (and ESP32-S2, ESP32-C3, ESP32-C6, ESP32-H2), use
  `esp_sleep_enable_ext1_wakeup(1ULL << gpio_num, ESP_EXT1_WAKEUP_ANY_LOW)` for active-LOW
  GPIO wakeup from a single pin. The wakeup cause is `ESP_SLEEP_WAKEUP_EXT1`, not `ESP_SLEEP_WAKEUP_EXT0`.
  The compiler error is: `implicit declaration of function 'esp_sleep_enable_ext0_wakeup'`. (Mar 2026)

- **NimBLE broadcaster-only builds require `CONFIG_BT_NIMBLE_SECURITY_ENABLE=n`** on ESP-IDF 5.5.x.
  Root cause: `ble_hs.c` calls `ble_sm_deinit()` when `NIMBLE_BLE_SM=1`, but `ble_sm.c` only
  defines `ble_sm_deinit()` when `BLE_STATIC_TO_DYNAMIC=1` (defaults to 0). This guard mismatch
  causes `undefined reference to 'ble_sm_deinit'` at link time. The fix is to disable the security
  manager entirely: `CONFIG_BT_NIMBLE_SECURITY_ENABLE=n`. This cascades to set `BLE_SM_LEGACY=0`
  and `BLE_SM_SC=0`, making `NIMBLE_BLE_SM=0`, so `ble_hs.c` never calls `ble_sm_deinit()`.
  A non-connectable broadcaster has zero use for the security manager.
  Also note: `sdkconfig.defaults` is only applied when no `sdkconfig` file exists at the project
  root. After any manual menuconfig session, delete `sdkconfig` before a clean build to ensure
  `sdkconfig.defaults` takes effect. (Mar 2026)

- **RTC GPIO ranges differ per ESP32 family — validate before assigning wakeup GPIOs.**
  `ext0`/`ext1` deep-sleep wakeup requires RTC-capable GPIOs. Valid ranges confirmed from
  ESP-IDF v5.5.3 `components/soc/<target>/include/soc/rtc_io_channel.h` and `esp_sleep.h` lines 314-318:
  - ESP32 (original): GPIO 0, 2, 4, 12–15, 25–27, 32–39
  - ESP32-S3: GPIO 0–21 (all RTC-capable)
  - **ESP32-C6: GPIO 0–7 only** — boot button GPIO 9 is NOT an RTC GPIO
  - **ESP32-C5: GPIO 0–6 only** — boot button GPIO 9 is NOT an RTC GPIO
  When a target's only physical button is not RTC-capable, use timer-only wakeup. No external
  hardware should be required as a workaround — document the limitation clearly instead.
  Symptoms of misassignment: `esp_sleep_enable_ext1_wakeup()` returns `ESP_ERR_INVALID_ARG`
  at runtime, or the device never wakes from deep sleep on button press. (Mar 2026)

- **`nvs_flash_init()` is required before any BLE/WiFi API call.** The BLE controller stores PHY
  calibration data in NVS. Without `nvs_flash_init()`, `esp_bt_controller_init()` fails with
  `BLE_INIT: controller init failed`. On USB CDC boards (XIAO ESP32S3), the resulting crash loop
  is invisible because the device resets too fast for USB to enumerate — the serial monitor shows
  only `waiting for download` and the device appears stuck. Always include:
  ```c
  #include "nvs_flash.h"
  // ...
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);
  ```
  Also add `nvs_flash` to `REQUIRES` in `main/CMakeLists.txt`. (Mar 2026)

- **`nimble_port_init()` handles BLE controller init — do NOT call `esp_bt_controller_init/enable` explicitly.**
  In ESP-IDF 5.5.3, `nimble_port_init()` internally calls `esp_bt_controller_init()` and
  `esp_bt_controller_enable()`. Calling them explicitly before `nimble_port_init()` causes a
  double-init crash (`BLE_INIT: controller init failed`). Similarly, `nimble_port_deinit()`
  internally calls `esp_bt_controller_disable()` and `esp_bt_controller_deinit()` — calling them
  explicitly after `nimble_port_deinit()` causes a double-deinit crash. The correct NimBLE
  lifecycle is: `nimble_port_init()` → use stack → `nimble_port_stop()` → `nimble_port_deinit()`.
  No `#include "esp_bt.h"` is needed. (Mar 2026)

- **ESP-IDF 5.5.x component name corrections for main/CMakeLists.txt REQUIRES:**
  - OTA operations (`esp_ota_get_running_partition`, `esp_ota_mark_app_valid_cancel_rollback`,
    `esp_ota_get_state_partition`) → component is **`app_update`**, NOT `esp_ota_ops`.
    `esp_ota_ops` does not exist as a standalone component; the header `esp_ota_ops.h` is
    provided by `app_update`. Using `esp_ota_ops` in REQUIRES causes cmake to fail with
    "component could not be found".
  - Task watchdog (`esp_task_wdt.h`) → header lives in **`esp_system`** component, not a
    standalone `esp_task_wdt` component. If you only need the WDT for implicit idle-task
    monitoring (sdkconfig CONFIG_ESP_TASK_WDT_EN=y), no explicit REQUIRES entry is needed —
    esp_system is already a transitive dependency. Only add `esp_system` to REQUIRES if you
    call `esp_task_wdt_add/reset/delete` directly.
  (Mar 2026)

- **YEJMKJ ESP32-S3-DevKitC-1-N16R8 uses native USB only — no CH343P bridge.**
  The board has a single USB-C port connected directly to the ESP32-S3 native USB OTG
  peripheral (GPIO19/GPIO20). It enumerates as a single port (`/dev/cu.usbmodem2101` on macOS)
  for both flashing and console output. Always set `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y` in
  `sdkconfig.defaults`. Flash and monitor together on the same port:
  `idf.py -p /dev/cu.usbmodem2101 flash monitor`.
  Do NOT set `CONFIG_ESP_CONSOLE_USB_CDC=y` and do NOT reference `usbserial-*` ports — there is
  no UART bridge on this board. (Mar 2026, corrected Mar 2026)

## Required Workflow When Something Goes Wrong
1. Read this file.
2. Check shared-specs/CodingStandards.md and the relevant board-spec.
3. Improve the FunctionalSpec.md or CodingSpec.md.
4. Regenerate with the full-project-generator skill.
5. Never manually edit generated code.

## CodingSpec.md Must Be High-Level Guidance (Not Implementation Instructions)

CodingSpec.md describes **what** and **why** — never **how**. It must not contain code snippets, `#define` statements, specific API call sequences, or numbered step-by-step instructions. It should describe architecture, constraints, preferred libraries and the reasoning behind them, non-functional requirements, trade-off decisions, and known gotchas.

The guiding principle: "The model we use today is the worst model we will ever use." A high-level spec benefits from better future models. A low-level spec just duplicates code in prose and locks the implementation to the current generation's idioms.

If you find yourself writing a function call, a struct definition, or a numbered sequence of API calls in CodingSpec.md, stop and reframe it as an architectural constraint or a trade-off decision. (Feb 2026)

## Template for Adding New Lessons
When you discover something important:
- Add it here under "Key Lessons Learned"
- Date it
- Make it actionable for future generations

This file is the living quality guardrail and institutional memory of the entire esp32-sdd-showcase project.
