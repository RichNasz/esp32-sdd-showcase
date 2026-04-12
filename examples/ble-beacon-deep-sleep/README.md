<!-- ================================================
     AGENT-GENERATED — DO NOT EDIT BY HAND
     Generated from specs/ using esp32-sdd-full-project-generator skill
     Date: 2026-04-12 | Agent: Claude Code
     ================================================ -->

# BLE Beacon + Deep Sleep

> Duty-cycled BLE beacon with per-cycle ANSI terminal dashboard and multi-year battery life.

## Overview

This example demonstrates BLE advertising with deep-sleep duty cycling. The device wakes from
deep sleep every 10 seconds, broadcasts a non-connectable BLE advertisement packet containing a
boot counter and a 4-byte magic payload for 10 seconds, then returns to deep sleep. The onboard
LED is lit only while BLE advertising is actually active — it remains dark during NimBLE
initialisation and teardown, so it accurately indicates RF activity rather than the overall wake
window. WS2812 boards show solid blue (the conventional BLE colour) at 64/255 brightness; simple
GPIO boards use the board's active level.

During the active window the serial monitor displays a live 6-row ANSI terminal dashboard showing
the cycle number, wakeup cause, BLE MAC address, advertising status, and a progress bar counting
down the 10-second advertising window. The dashboard refreshes every 250 ms using the per-cycle
snapshot pattern (cursor-home + overwrite) from the `esp32-ansi-monitor-engineer` skill.

The beacon is identifiable by name (`"ESP32-SDD"`) in BLE scanner apps without MAC filtering.

## Prerequisites

- **ESP-IDF**: v5.x (tested on v5.5.3)
- **Supported boards**:

  | Board | idf.py Target | LED GPIO | LED Type | Button Wakeup | Console | Role |
  |---|---|---|---|---|---|---|
  | YEJMKJ ESP32-S3-DevKitC-1-N16R8 | `esp32s3` | GPIO 48 | WS2812 RGB | ✗ timer-only | USB Serial/JTAG | **primary** |
  | Seeed XIAO ESP32S3 | `esp32s3` | GPIO 21 (active LOW) | Simple GPIO | ✓ GPIO 0 ext1 | USB CDC | secondary |
  | Espressif ESP32-C6-DevKitC-1-N8 | `esp32c6` | GPIO 8 | WS2812 RGB | ✗ timer-only | UART (CH343P) | **primary** |
  | Seeed XIAO ESP32-C6 | `esp32c6` | GPIO 15 (active LOW) | Simple GPIO | ✗ timer-only | USB Serial/JTAG | secondary |
  | Seeed XIAO ESP32-C5 | `esp32c5` | GPIO 27 (active LOW) | Simple GPIO | ✗ timer-only | USB Serial/JTAG | sole |
  | Adafruit HUZZAH32 | `esp32` | GPIO 13 (active HIGH) | Simple GPIO | ✓ GPIO 0 ext0 | UART (CP2104) | sole |

  **Primary** boards have their configuration baked into `sdkconfig.defaults.<target>`.
  **Secondary** boards require a one-time `idf.py menuconfig` step after `set-target` (see below).

- **BLE scanner app**: nRF Connect (iOS/Android) or equivalent — for manual advertisement verification.

## Build & Flash

### YEJMKJ ESP32-S3-DevKitC-1-N16R8 (primary esp32s3)

```sh
cd examples/ble-beacon-deep-sleep
idf.py set-target esp32s3
idf.py build flash monitor
```

The YEJMKJ board uses USB Serial/JTAG. The serial port stays connected across sleep cycles on
most host systems.

### Seeed XIAO ESP32S3 (secondary esp32s3)

```sh
idf.py set-target esp32s3
idf.py menuconfig
```

In `BLE Beacon Configuration`, set:
- `Use WS2812 RGB LED` → **Disabled**
- `LED GPIO number` → **21**
- `LED active level` → **0** (active LOW)

Also in `Component config → ESP System Settings → Channel for console output` → **USB CDC**.

```sh
idf.py build flash
```

Open the serial monitor in a separate terminal (monitor requires an interactive TTY):

```sh
idf.py monitor
```

> **Note**: The XIAO ESP32S3 uses USB CDC for serial output. The serial port
> (`/dev/cu.usbmodem*`) **disappears when the chip enters deep sleep** — this is expected.
> The dashboard freezes on the `SLEEPING` frame; the terminal reconnects automatically on
> the next wakeup. For multi-cycle serial observation, use the Adafruit HUZZAH32 instead.

### Espressif ESP32-C6-DevKitC-1-N8 (primary esp32c6)

```sh
cd examples/ble-beacon-deep-sleep
idf.py set-target esp32c6
idf.py build flash monitor
```

The ESP32-C6-DevKitC uses a CH343P UART bridge that remains powered during deep sleep.
The serial port stays connected across sleep cycles.

### Seeed XIAO ESP32-C6 (secondary esp32c6)

```sh
idf.py set-target esp32c6
idf.py menuconfig
```

In `BLE Beacon Configuration`, set:
- `Use WS2812 RGB LED` → **Disabled**
- `LED GPIO number` → **15**
- `LED active level` → **0** (active LOW)

```sh
idf.py build flash monitor
```

### Seeed XIAO ESP32-C5

```sh
idf.py set-target esp32c5
idf.py build flash monitor
```

### Adafruit HUZZAH32

```sh
idf.py set-target esp32
idf.py build flash monitor
```

The HUZZAH32 uses a CP2104 UART bridge chip that remains powered during deep sleep. The serial
port stays connected across sleep cycles — ideal for observing multiple consecutive cycles.

## Opening in VS Code / Cursor

This folder is a complete, standalone ESP-IDF project. Open **only this folder**
(not the repository root) in VS Code or Cursor so the official ESP-IDF Extension
can detect and configure the project automatically.

**File → Open Folder… → select `examples/ble-beacon-deep-sleep/`**

This gives you one-click Build / Flash / Monitor / Debug, Menuconfig, and size
analysis from the ESP-IDF commands palette. Do not open the top-level repository
folder as your active workspace while developing this example.

## Expected Serial Output

On first flash, ESP-IDF startup logs appear briefly, then the screen clears and the ANSI
dashboard takes over. The dashboard remains visible for the full 10 s advertising window,
then the device enters deep sleep.

During BLE initialisation (`INITIALIZING` — LED dark):

```
  BLE BEACON MONITOR   ESP32-SDD   cycle #1
  Cycle: #1      Wakeup: FIRST_BOOT
  BLE MAC: --:--:--:--:--:--
  Status:  INITIALIZING
  Active:  [                    ] -- s remaining
  ------------------------------------------------
```

Once NimBLE syncs and advertising begins (`ADVERTISING` — LED on):

```
  BLE BEACON MONITOR   ESP32-SDD   cycle #1
  Cycle: #1      Wakeup: FIRST_BOOT
  BLE MAC: 3C:84:27:F0:AB:CD
  Status:  ADVERTISING
  Active:  [=======             ] 6.5 s remaining
  ------------------------------------------------
```

After the advertising window ends (`SLEEPING` — LED off):

```
  BLE BEACON MONITOR   ESP32-SDD   cycle #1
  Cycle: #1      Wakeup: FIRST_BOOT
  BLE MAC: 3C:84:27:F0:AB:CD
  Status:  SLEEPING
  Active:  [====================] 0.0 s remaining
  ------------------------------------------------
```

On subsequent wakeups (UART bridge boards — port stays connected across sleep):

```
  BLE BEACON MONITOR   ESP32-SDD   cycle #2
  Cycle: #2      Wakeup: TIMER
  BLE MAC: 3C:84:27:F0:AB:CD
  Status:  ADVERTISING
  Active:  [==                  ] 8.8 s remaining
  ------------------------------------------------
```

> **USB-native boards** (XIAO ESP32S3, XIAO ESP32-C6, XIAO ESP32-C5): the serial port
> disappears when the chip sleeps; `idf.py monitor` disconnects on the `SLEEPING` frame.
> Use the **Adafruit HUZZAH32** or **ESP32-C6-DevKitC-1-N8** for multi-cycle monitoring —
> their UART bridges stay powered during sleep.

## Key Concepts

- **LED indicates RF activity, not wake state** — `led_adv_on()` is called only after
  `ble_gap_adv_start()` returns success; `led_adv_off()` is called immediately in the
  advertising-complete callback (`adv_timer_cb`) after `ble_gap_adv_stop()`. The LED
  remains dark during NimBLE init and teardown.
- **WS2812 solid blue for BLE** — `ws2812_write(0, 0, 64)` while advertising is active.
  Blue is the conventional colour for BLE activity; 64/255 meets the minimum brightness floor.
  The RMT `eot_level=0` holds the data line LOW after each frame — no `gpio_hold_en()` needed.
- **GPIO `gpio_hold_en()` / `gpio_hold_dis()`** — deep sleep latches GPIO states. For simple
  GPIO boards: `gpio_hold_dis(LED_GPIO)` in `led_init()` at wake, `gpio_hold_en(LED_GPIO)` in
  `app_main` just before `esp_deep_sleep_start()`.
- **Per-cycle snapshot dashboard** — ANSI TUI pattern using cursor-home (`ESC[H`) + overwrite
  on each 250 ms refresh tick. No scroll region is needed for duty-cycled firmware; the screen
  naturally freezes during deep sleep and redraws cleanly on the next wakeup.
- **`esp_rom_printf()`** — ROM UART write function that bypasses newlib stdio and the VFS layer,
  guaranteeing zero-buffered output. Used exclusively for all dashboard output after the initial
  screen clear. Pre-formatted via `snprintf` into a stack buffer first.
- **`esp_log_level_set("*", ESP_LOG_NONE)`** — suppresses all ESP-IDF log output after the
  first dashboard frame is drawn. Status and errors are shown in the dashboard Status field.
- **Task notification wakeup** — both the 250 ms refresh timer and the 10 s advertising-window
  timer use `xTaskNotifyGive()` to wake the main task. The polling loop calls
  `xTaskNotifyWait()` with a 300 ms timeout and redraws on each wakeup.
- **NimBLE broadcaster-only** — `CONFIG_BT_NIMBLE_SECURITY_ENABLE=n` required on ESP-IDF 5.5.x
  to avoid `undefined reference to 'ble_sm_deinit'` at link time. See `sdkconfig.defaults`.
- **3-step BLE teardown** — `ble_gap_adv_stop()` → `nimble_port_stop()` → `nimble_port_deinit()`.
  `nimble_port_deinit()` handles controller disable/deinit internally on ESP-IDF 5.5.x.
  Never call `esp_bt_controller_disable/deinit()` explicitly — causes a double-deinit crash.
- **RTC-capable GPIO restriction** — ESP32-C6 (GPIO 0–7 only) and ESP32-C5 (GPIO 0–6 only)
  cannot use GPIO 9 (boot button) as a wakeup source. Timer-only wakeup on those targets.
- **`nvs_flash_init()` before BLE** — the BLE controller stores PHY calibration in NVS.
  Omitting this causes `esp_bt_controller_init()` to fail with a cryptic controller abort.
- **Primary/secondary board pattern** — `sdkconfig.defaults.<target>` configures the primary
  board (WS2812 DevKitC variants). Secondary boards (XIAO variants) require `idf.py menuconfig`
  to override LED GPIO, LED type, and active level after `set-target`.

## Testing

Testing philosophy: automated first, manual only when hardware observation is unavoidable.

### Automated Tests

1. **T-A1 — Zero-warning build (esp32s3)**

   ```sh
   cd examples/ble-beacon-deep-sleep
   idf.py set-target esp32s3 && idf.py build
   ```

   Pass: exit code 0, zero compiler warnings.

2. **T-A2 — Binary size check**

   ```sh
   idf.py size | grep "Total binary size"
   ```

   Pass: reported size < 1.5 MB (NimBLE stack included).

3. **T-A3 — NimBLE config check**

   ```sh
   grep "CONFIG_BT_NIMBLE_ENABLED=y" examples/ble-beacon-deep-sleep/sdkconfig.defaults
   ```

   Pass: line present.

### Manual — hardware required

4. **T-M1 — BLE advertisement visible to scanner**

   *Why manual*: Verifying OTA BLE advertisement content requires a second device acting as a
   scanner — not reproducible in simulation.

   Prerequisites: smartphone with nRF Connect (iOS/Android) or equivalent BLE scanner.

   1. Flash to YEJMKJ ESP32-S3-DevKitC-1-N16R8: `idf.py set-target esp32s3 && idf.py build flash`
   2. Open nRF Connect → Scanner tab → start scanning.
   3. On each wake cycle, confirm a device named `"ESP32-SDD"` appears in the scan list.
   4. Inspect raw manufacturer data — confirm company ID `0xFFFF` and magic `DE AD BE EF`.
   5. Confirm `boot_count` field increments by 1 on each wake.

   Pass: advertisement appears on every cycle; manufacturer data matches spec.

5. **T-M2 — Active window ≤ 200 ms**

   *Why manual*: Precise active window timing requires a logic analyser or oscilloscope, or
   observation of LED blink duration.

   1. Observe LED blink during advertising window.
   2. LED blink must be brief (≈100 ms); device must return to sleep quickly.
   3. Serial monitor must show adv-start then adv-done within the same second timestamp.

   Pass: LED blink ≈ 100 ms; total active window ≤ 200 ms per serial timestamps.

6. **T-M3 — ANSI dashboard renders correctly**

   *Why manual*: Visual terminal output (colour, layout, refresh rate) requires human observation.

   1. Flash to Adafruit HUZZAH32: `idf.py set-target esp32 && idf.py build flash monitor`
   2. Confirm 6-row dashboard appears after the initial startup log spam is cleared.
   3. Confirm BLE MAC row shows `--:--:--:--:--:--` during `INITIALIZING`, then a valid MAC.
   4. Confirm Status row transitions: `INITIALIZING` → `ADVERTISING` → `SLEEPING`.
   5. Confirm progress bar fills left-to-right over ~10 seconds and freezes full on `SLEEPING`.
   6. Confirm screen redraws on the next wakeup with cycle #2 and `Wakeup: TIMER`.

   Pass: dashboard renders cleanly; all rows update correctly; no garbled escape sequences.

7. **T-M4 — Button wakeup triggers immediate advertisement (ESP32 / ESP32-S3 only)**

   *Why manual*: Physical GPIO wakeup requires real hardware.

   1. While device is in deep sleep, press and release the BOOT button (GPIO 0).
   2. Confirm the dashboard appears within 500 ms of the button press.
   3. Confirm `Wakeup:` field shows `BUTTON (EXT0)` (HUZZAH32) or `BUTTON (EXT1)` (XIAO S3).

   Pass: dashboard appears within 500 ms; correct wakeup cause displayed.

## Spec Files

- [specs/FunctionalSpec.md](specs/FunctionalSpec.md) — requirements, board table, power budget
- [specs/CodingSpec.md](specs/CodingSpec.md) — architecture, dashboard spec, NimBLE teardown
- [specs/TestSpec.md](specs/TestSpec.md) — automated and manual test procedures

---

*Generated by [esp32-sdd-full-project-generator](../../skills/esp32-sdd-full-project-generator/SKILL.md)*
