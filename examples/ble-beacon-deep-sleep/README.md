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
boot counter and a 4-byte magic payload for 10 seconds, then returns to deep sleep. During the
active window the onboard LED is lit, and the serial monitor displays a live 6-row ANSI terminal
dashboard showing the cycle number, wakeup cause, BLE MAC address, advertising status, and a
progress bar counting down the 10-second advertising window. The dashboard refreshes every 250 ms
using the per-cycle snapshot pattern (cursor-home + overwrite) from the
`esp32-ansi-monitor-engineer` skill.

The beacon is identifiable by name (`"ESP32-SDD"`) in BLE scanner apps without MAC filtering.

## Prerequisites

- **ESP-IDF**: v5.x (tested on v5.5.3)
- **Supported boards**:

  | Board | idf.py Target | LED GPIO | Button Wakeup | Console |
  |---|---|---|---|---|
  | Seeed XIAO ESP32S3 | `esp32s3` | GPIO 21 (active LOW) | ✓ GPIO 0 ext1 | USB CDC |
  | Seeed XIAO ESP32-C6 | `esp32c6` | GPIO 15 (active LOW) | ✗ timer-only | USB Serial/JTAG |
  | Seeed XIAO ESP32-C5 | `esp32c5` | GPIO 27 (active LOW) | ✗ timer-only | USB Serial/JTAG |
  | Adafruit HUZZAH32 | `esp32` | GPIO 13 (active HIGH) | ✓ GPIO 0 ext0 | UART (CP2104) |

- **BLE scanner app**: nRF Connect (iOS/Android) or equivalent — for manual advertisement verification.

## Build & Flash

### Seeed XIAO ESP32S3

```sh
cd examples/ble-beacon-deep-sleep
idf.py set-target esp32s3
idf.py build
idf.py flash
```

Open the serial monitor in a separate terminal (monitor requires an interactive TTY):

```sh
idf.py monitor
```

> **Note**: The XIAO ESP32S3 uses USB CDC for serial output. The serial port
> (`/dev/cu.usbmodem*`) **disappears when the chip enters deep sleep** — this is expected.
> The dashboard freezes on the `SLEEPING` frame; the terminal reconnects automatically on
> the next wakeup. For multi-cycle serial observation, use the Adafruit HUZZAH32 instead.

### Adafruit HUZZAH32

```sh
idf.py set-target esp32
idf.py build flash monitor
```

The HUZZAH32 uses a CP2104 UART bridge chip that remains powered during deep sleep. The serial
port stays connected across sleep cycles — ideal for observing multiple consecutive cycles.

### Seeed XIAO ESP32-C6

```sh
idf.py set-target esp32c6
idf.py build flash monitor
```

### Seeed XIAO ESP32-C5

```sh
idf.py set-target esp32c5
idf.py build flash monitor
```

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

```
  BLE BEACON MONITOR   ESP32-SDD   cycle #1
  Cycle: #1      Wakeup: FIRST_BOOT
  BLE MAC: 3C:84:27:F0:AB:CD
  Status:  ADVERTISING
  Active:  [=======             ] 6.5 s remaining
  ------------------------------------------------
```

After the advertising window ends:

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
> Use the **Adafruit HUZZAH32** for multi-cycle monitoring — its CP2104 bridge stays
> powered during sleep.

## Key Concepts

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
- **`gpio_hold_dis()` / `gpio_hold_en()`** — deep sleep latches GPIO states. Call
  `gpio_hold_dis()` before driving the LED on each wake; `gpio_hold_en()` before sleeping.
- **RTC-capable GPIO restriction** — ESP32-C6 (GPIO 0–7 only) and ESP32-C5 (GPIO 0–6 only)
  cannot use GPIO 9 (boot button) as a wakeup source. Timer-only wakeup on those targets.
- **`nvs_flash_init()` before BLE** — the BLE controller stores PHY calibration in NVS.
  Omitting this causes `esp_bt_controller_init()` to fail with a cryptic controller abort.

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

   1. Flash to XIAO ESP32S3: `idf.py set-target esp32s3 && idf.py build flash`
   2. Open nRF Connect → Scanner tab → start scanning.
   3. On each wake cycle, confirm a device named `"ESP32-SDD"` appears in the scan list.
   4. Inspect raw manufacturer data — confirm company ID `0xFFFF` and magic `DE AD BE EF`.
   5. Confirm `boot_count` field increments by 1 on each wake.

   Pass: advertisement appears on every cycle; manufacturer data matches spec.

5. **T-M2 — ANSI dashboard renders correctly**

   *Why manual*: Visual terminal output (colour, layout, refresh rate) requires human observation.

   1. Flash to Adafruit HUZZAH32: `idf.py set-target esp32 && idf.py build flash monitor`
   2. Confirm 6-row dashboard appears after the initial startup log spam is cleared.
   3. Confirm BLE MAC row shows `--:--:--:--:--:--` briefly, then a valid MAC after sync.
   4. Confirm Status row transitions: `INITIALIZING` → `ADVERTISING` → `SLEEPING`.
   5. Confirm progress bar fills left-to-right over ~10 seconds and freezes full on `SLEEPING`.
   6. Confirm screen redraws on the next wakeup with cycle #2 and `Wakeup: TIMER`.

   Pass: dashboard renders cleanly; all rows update correctly; no garbled escape sequences.

6. **T-M3 — Button wakeup triggers immediate advertisement (ESP32 / ESP32-S3 only)**

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
