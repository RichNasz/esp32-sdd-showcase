---
name: esp32-ansi-monitor-engineer
description: Generates ANSI TUI dashboard code for ESP32 serial monitor output. Supports two
  patterns: Pattern A (continuous dashboard with DECSTBM scroll region, for always-on firmware)
  and Pattern B (per-cycle snapshot with cursor-home + overwrite, for duty-cycled/deep-sleep
  firmware). Emits escape definitions, drawing functions, refresh timer wiring, log suppression,
  and task notification discipline.
version: "1.0.0"
author: Richard Naszcyniec
keywords: sdd, esp32, ansi, tui, terminal, dashboard, monitor, serial, esp_rom_printf
---

<!-- ================================================
     AGENT-GENERATED — DO NOT EDIT BY HAND
     Generated from specs/ using esp32-sdd-documentation-generator skill
     Date: 2026-04-12 | Agent: Claude Code
     ================================================ -->

# ESP32 ANSI Monitor Engineer

## When to use

Any example where the serial monitor output should be a structured terminal dashboard rather than
plain `ESP_LOGI` lines. Invoke standalone to add a dashboard to an existing example, or as a
sub-workflow of `esp32-sdd-full-project-generator` when the CodingSpec references this skill.

## Pattern Selection

Choose one pattern based on the firmware's execution model:

**Pattern A — Continuous Dashboard (scroll region)**
- Firmware runs continuously with a live event stream (ESP-NOW gateway, WiFi server, MQTT broker).
- New events arrive in real time and must appear as scrolling lines below a pinned header.
- Use DECSTBM scroll region (`ESC[top;botr`) to pin header rows; new event lines scroll below.
- Reference example: `examples/esp-now-low-power-mesh/main/gateway.c`

**Pattern B — Per-Cycle Snapshot (cursor-home + overwrite)**
- Firmware is duty-cycled: discrete active windows separated by deep sleep or idle periods.
- Each cycle produces a fixed set of data fields; no continuous scrolling event log needed.
- Use cursor-home (`ESC[H`) + overwrite on each refresh tick; screen freezes during sleep.
- Reference example: `examples/ble-beacon-deep-sleep/main/main.c`

**Decision rule**: If the firmware enters deep sleep between cycles → Pattern B. If the firmware
runs indefinitely and must display a continuous stream of incoming events → Pattern A.

## Workflow (follow exactly)

MANDATORY: First read and follow `shared-specs/AIGenLessonsLearned.md` before doing anything else.

1. **Read the CodingSpec** for the target example. Confirm which pattern is specified. If the
   CodingSpec specifies a dashboard layout (fields, rows, refresh rate), use it exactly. If it
   does not, choose the pattern from the decision rule above and document the choice.

2. **Emit the ANSI escape `#define` block** near the top of the source file, before any function
   definitions. Use these exact definitions verbatim — do not abbreviate or reorder:

   ```c
   /* -----------------------------------------------------------------------
    * ANSI escape sequences
    *
    * All post-init terminal output goes through esp_rom_printf(), which writes
    * directly to the UART ROM routine — bypassing newlib stdio and the VFS
    * layer entirely, guaranteeing every byte appears immediately with zero
    * buffering. Pre-format with snprintf into a stack buffer first, then emit
    * with esp_rom_printf("%s", buf) to avoid relying on ROM printf specifier
    * support (width/precision may be absent in some ESP-IDF ROM builds).
    * ----------------------------------------------------------------------- */
   #define ESC              "\033"

   /* Cursor positioning */
   #define ANSI_HOME        ESC "[H"           /* cursor to row 1, col 1 (Pattern B) */
   #define ANSI_CLEAR       ESC "[2J" ESC "[H" /* clear screen + home (used once on init) */
   #define ANSI_ERASE_EOL   ESC "[K"           /* erase from cursor to end of line */
   #define ANSI_CURSOR_OFF  ESC "[?25l"        /* DECTCEM — hide cursor */

   /* Pattern A only — DEC cursor save/restore
    * Use \0337/\0338 (DEC), NOT \033[s/\033[u (SCO).
    * SCO sequences are unsupported in macOS Terminal.app; DEC works in all terminals. */
   #define ANSI_SAVE        "\0337"            /* DEC ESC 7 — save cursor + SGR state */
   #define ANSI_RESTORE     "\0338"            /* DEC ESC 8 — restore cursor + SGR state */

   /* Pattern A only — DECSTBM scroll region.
    * Adjust the top row number to match the header height. */
   #define ANSI_SCROLL_LOCK ESC "[6;500r"      /* pin rows 1-5; scroll region = row 6+ */

   /* Colour and style */
   #define C_RESET  ESC "[0m"
   #define C_BOLD   ESC "[1m"
   #define C_DIM    ESC "[2m"
   #define C_GREEN  ESC "[32m"
   #define C_YELLOW ESC "[33m"
   #define C_RED    ESC "[31m"
   #define C_CYAN   ESC "[36m"
   #define C_GRAY   ESC "[90m"
   #define C_WHITE  ESC "[97m"
   #define C_BGBLU  ESC "[44m"
   ```

3. **Add module-scope dashboard state** — variables that are written by callbacks but only
   read by the drawing function. Declare them as `static` at file scope with clear names.
   Typical fields: MAC address buffer, advertising status enum, cycle start timestamp,
   main task handle. The drawing function reads these; callbacks only write them.

4. **Implement the drawing function(s)** according to the chosen pattern (see Pattern A and
   Pattern B workflows below). Adhere strictly to the output discipline rules in step 6.

5. **Wire up the refresh timer and task notification**:
   - Create a periodic `esp_timer` at the refresh interval specified in the CodingSpec
     (default: 250 ms if not specified).
   - Timer callback body: `xTaskNotifyGive(s_main_task_handle)` and nothing else. No screen
     output in callbacks.
   - Store `s_main_task_handle = xTaskGetCurrentTaskHandle()` before creating the timer.

6. **Apply the mandatory output discipline rules** — add these as source comments where the
   rules apply:
   - All post-init terminal output via `esp_rom_printf()` only — never `printf`, `fprintf`,
     or `ESP_LOG*` macros after the dashboard is drawn.
   - `snprintf` to a stack buffer first, then `esp_rom_printf("%s", buf)`. This avoids ROM
     printf format specifier limitations and keeps output atomic.
   - Call `esp_log_level_set("*", ESP_LOG_NONE)` immediately after clearing the screen and
     drawing the first frame. After this call, ESP-IDF log suppression is permanent for the
     rest of the wake cycle. Error conditions must be reflected in a `Status` field in the
     dashboard, not via `ESP_LOGE`.
   - Only the task that owns the screen writes to the terminal. In single-task firmware
     (Pattern B), only `app_main` writes. In multi-task firmware (Pattern A), designate one
     task as screen owner. All other tasks only call `xTaskNotifyGive()` or set atomic flags.

7. **Add `esp_rom_sys.h` include**. Add it with the other system includes:

   ```c
   #include "esp_rom_sys.h"   /* esp_rom_printf() — zero-buffered ROM UART output */
   ```

   The `esp_rom` component is typically a transitive dependency. If the linker reports
   `undefined reference to 'esp_rom_printf'`, add `esp_rom` explicitly to REQUIRES in
   `main/CMakeLists.txt`.

8. **Update `main/CMakeLists.txt`** if needed (only when `esp_rom` must be added to REQUIRES).

9. Verify the dashboard output by inspecting `idf.py monitor` after flashing. Confirm:
   - All rows appear on first boot without corruption.
   - Colours and bold/dim styling render correctly in macOS Terminal.app.
   - Refresh rate matches the CodingSpec (no perceptible flicker).
   - `ESP_LOGI` lines from the NimBLE or WiFi stack do not appear after log suppression.

---

## Pattern A Workflow — Continuous Dashboard (scroll region)

Use for always-on firmware with a continuous incoming event stream.

### draw_header() — called once after peripheral init

```
ANSI_CLEAR                     — clear screen
Draw rows 1..N as fixed header:
  Row 1: title bar (C_BGBLU C_BOLD C_WHITE)
  Row 2: status row (left empty; draw_status() populates it)
  Row 3: separator line (C_GRAY dashes)
  Row 4: column header labels (C_DIM)
  Row 5: separator line (C_GRAY dashes)
ANSI_SCROLL_LOCK               — pin rows 1-5; scroll region = row 6+
Position cursor at row 6       — enter scroll region
ANSI_CURSOR_OFF                — hide cursor
draw_status()                  — populate the status row immediately
```

After `draw_header()`, call `esp_log_level_set("*", ESP_LOG_NONE)`.

### draw_status() — called whenever status data changes

Overwrites the status row in-place without disturbing the scroll cursor:

```
ANSI_SAVE                      — save current cursor position
Position at status row (e.g. ESC[2;1H)
Write status content
ANSI_ERASE_EOL                 — clear any stale trailing characters
ANSI_RESTORE                   — restore cursor to prior position
```

Key: overwrite content *before* ERASE_EOL to avoid a visible blank flash at 115200 baud.

### print_event_line() — called per incoming event

Pre-format the entire line into a stack buffer with `snprintf`, then:

```c
esp_rom_printf("%s\r\n", line);   /* CR returns to col 0; LF advances the scroll row */
```

The scroll region ensures new lines scroll the content while the header stays fixed.

### Main task loop (Pattern A)

```
xTaskNotifyWait(...)            — block until callback wakes us
Check notification source (enum or bit flags)
If status changed: draw_status()
If new event: print_event_line()
```

---

## Pattern B Workflow — Per-Cycle Snapshot (cursor-home + overwrite)

Use for duty-cycled firmware (deep sleep between active windows).

### draw_dashboard(bool initial) — called every ~250 ms

On `initial = true` (first call only):
```
ANSI_CLEAR                     — clear screen once (avoids flicker on all subsequent draws)
ANSI_CURSOR_OFF                — hide cursor
esp_log_level_set("*", ESP_LOG_NONE)  — suppress ESP-IDF logs permanently
```

On every call (including initial):
```
ANSI_HOME                      — cursor to row 1 (NOT ANSI_CLEAR — avoids full-screen flicker)
For each row 1..N:
  Position at row start
  Write row content
  ANSI_ERASE_EOL               — clear stale trailing characters from previous draw
```

Write content first, then `ANSI_ERASE_EOL`. Never blank the line first — that causes a visible
8 ms flash at 115200 baud while new content fills in.

### Module-scope state written by callbacks, read by draw_dashboard()

```c
static volatile uint8_t  s_adv_status;    /* enum: INIT, ADVERTISING, SLEEPING */
static uint8_t           s_ble_mac[6];    /* populated by ble_sync_cb */
static int64_t           s_cycle_start_us;/* set when advertising starts */
static volatile bool     s_adv_done;      /* set by adv_timer_cb */
static TaskHandle_t      s_main_task_handle;
```

### Progress bar computation

```
elapsed_us  = esp_timer_get_time() - s_cycle_start_us
window_us   = ADV_WINDOW_US (e.g. 10 000 000)
fraction    = elapsed_us / window_us  (clamp to [0.0, 1.0])
filled      = (int)(fraction * BAR_WIDTH)  (e.g. BAR_WIDTH = 20)
```

Fill character: `=` (ASCII) or `═` (U+2550, box-drawing double horizontal). The CodingSpec
specifies which character to use. If not specified, use `=`.

### Replacing the blocking semaphore wait with a polling loop

The main task's blocking `xSemaphoreTake(s_adv_done_sem, portMAX_DELAY)` must become a polling
loop so the dashboard can refresh. The replacement pattern:

```c
/* Record start time for progress bar */
s_cycle_start_us = esp_timer_get_time();

/* Create 250 ms periodic refresh timer */
/* (timer callback: xTaskNotifyGive(s_main_task_handle)) */

/* Polling loop — redraws dashboard on each tick */
while (!s_adv_done) {
    xTaskNotifyWait(0, ULONG_MAX, NULL, pdMS_TO_TICKS(300));
    draw_dashboard(false);
}

/* Delete refresh timer */
/* Draw final SLEEPING frame */
draw_dashboard(false);   /* s_adv_status already set to SLEEPING by adv_timer_cb */
```

The `adv_timer_cb` callback sets `s_adv_done = true`, sets `s_adv_status = SLEEPING`, and calls
`xTaskNotifyGive(s_main_task_handle)` — it does NOT call `xSemaphoreGive` (the semaphore is
replaced by the flag + task notification).

### adv_timer_cb changes

Replace:
```c
ble_gap_adv_stop();
xSemaphoreGive(s_adv_done_sem);
```

With:
```c
ble_gap_adv_stop();
s_adv_status = ADV_STATUS_SLEEPING;
s_adv_done   = true;
xTaskNotifyGive(s_main_task_handle);
```

---

## Terminal Compatibility Notes

- **macOS Terminal.app**: Supports ANSI colours, DECTCEM (`ESC[?25l`/`ESC[?25h`), DECSTBM, and
  DEC save/restore (`\0337`/`\0338`). Does NOT support SCO save/restore (`ESC[s`/`ESC[u`).
- **iTerm2**: Supports all of the above plus SCO sequences. Always prefer DEC over SCO for
  portability.
- **`idf.py monitor` (ESP-IDF)**: Passes ANSI sequences through to the host terminal. ESP-IDF
  5.x does not strip or interpret ANSI escapes in monitor mode.
- **PlatformIO Serial Monitor**: May or may not interpret ANSI sequences depending on the
  terminal emulator in use. This skill targets `idf.py monitor` as the reference terminal.
