---
name: esp32-ansi-monitor-engineer
description: Generates ANSI TUI dashboard code for ESP32 serial monitor output. Supports three
  patterns: Pattern A (continuous dashboard with DECSTBM scroll region, for always-on firmware
  with a live event stream), Pattern B (per-cycle snapshot with periodic timer, for duty-cycled
  or deep-sleep firmware), and Pattern B-Lifecycle (sequential pipeline with lifecycle-driven
  redraws, for always-on firmware with discrete sequential phases and no continuous event stream).
  Emits escape definitions, drawing functions, refresh wiring, log suppression, and task
  notification discipline.
version: "1.1.0"
author: Richard Naszcyniec
keywords: sdd, esp32, ansi, tui, terminal, dashboard, monitor, serial, esp_rom_printf
---

<!-- ================================================
     AGENT-GENERATED — DO NOT EDIT BY HAND
     Generated from specs/ using esp32-sdd-documentation-generator skill
     Date: 2026-04-13 | Agent: Claude Code
     ================================================ -->

# ESP32 ANSI Monitor Engineer

## When to use

Any example where the serial monitor output should be a structured terminal dashboard rather than
plain `ESP_LOGI` lines. Invoke standalone to add a dashboard to an existing example, or as a
sub-workflow of `esp32-sdd-full-project-generator` when the CodingSpec references this skill.

## Pattern Selection

Choose one pattern based on the firmware's execution model:

**Pattern A — Continuous Dashboard (scroll region)**
- Firmware runs indefinitely with a real-time incoming event stream (ESP-NOW gateway, Wi-Fi
  server, MQTT broker).
- New events must appear as scrolling lines below a pinned header.
- Use DECSTBM scroll region (`ESC[top;botr`) to pin header rows; new event lines scroll below.
- Reference example: `examples/esp-now-low-power-mesh/main/gateway.c`

**Pattern B — Per-Cycle Snapshot (cursor-home + overwrite, timer-driven)**
- Firmware is duty-cycled: discrete active windows separated by deep sleep or idle periods.
- Each wake cycle produces a fixed set of data fields; screen freezes during sleep.
- A periodic `esp_timer` (250 ms) drives redraws within each active window via task notification.
- Reference example: `examples/ble-beacon-deep-sleep/main/main.c`

**Pattern B-Lifecycle — Sequential Pipeline (cursor-home + overwrite, lifecycle-driven)**
- Firmware runs continuously but progresses through discrete sequential phases
  (e.g. `BOOTING → CONNECTING → READY → DOWNLOADING → COMPLETE`).
- No continuous event stream; no deep sleep. Phase transitions naturally drive redraws.
- No periodic `esp_timer` needed — each phase calls `draw_dashboard(false)` at appropriate points.
- For blocking operations within a phase, use polling loop substitution or threshold-based redraws
  (see Pattern B-Lifecycle Workflow below).
- Reference example: `examples/secure-ota-https/main/main.c`

**Decision rule:**

| Firmware model | Pattern |
|---|---|
| Always-on, real-time event stream (events arrive continuously) | **A** |
| Duty-cycled with deep sleep between active windows | **B** |
| Always-on, sequential discrete phases, fixed field set, no streaming events | **B-Lifecycle** |

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

4. **Implement the drawing function(s)** according to the chosen pattern (see Pattern A,
   Pattern B, and Pattern B-Lifecycle workflows below). Adhere strictly to the output
   discipline rules in step 6.

5. **Wire up the refresh mechanism** — the approach differs by pattern:

   **Pattern A and Pattern B (timer-driven):**
   - Create a periodic `esp_timer` at the refresh interval specified in the CodingSpec
     (default: 250 ms if not specified).
   - Timer callback body: `xTaskNotifyGive(s_main_task_handle)` and nothing else. No screen
     output in callbacks.
   - Store `s_main_task_handle = xTaskGetCurrentTaskHandle()` before creating the timer.

   **Pattern B-Lifecycle (lifecycle-driven):**
   - No `esp_timer` is needed. Each phase transition calls `draw_dashboard(false)` directly.
   - `s_main_task_handle` is not needed (there is no timer callback to wake the main task).
   - For blocking operations that must show live progress within a phase, use polling loop
     substitution or threshold-based redraws — see Pattern B-Lifecycle Workflow below.

6. **Apply the mandatory output discipline rules** — add these as source comments where the
   rules apply:

   - All post-init terminal output via `esp_rom_printf()` only — never `printf`, `fprintf`,
     or `ESP_LOG*` macros after the dashboard is drawn.
   - `snprintf` to a stack buffer first (`char buf[256]`), then `esp_rom_printf("%s", buf)`.
     Use 256 bytes as the default buffer size — ANSI escape sequences add invisible bytes
     that do not appear as terminal output but do count toward the buffer. A 128-byte buffer
     can overflow when a single row contains multiple colour/reset sequences plus a long
     string argument.
   - Call `esp_log_level_set("*", ESP_LOG_NONE)` immediately after clearing the screen and
     drawing the first frame. After this call, ESP-IDF log suppression is permanent for the
     rest of the wake cycle. Error conditions must be reflected in a `Status` field in the
     dashboard, not via `ESP_LOGE`.
   - Only the task that owns the screen writes to the terminal. In single-task firmware
     (Pattern B, Pattern B-Lifecycle), only `app_main` writes. In multi-task firmware
     (Pattern A), designate one task as screen owner. All other tasks only call
     `xTaskNotifyGive()` or set atomic flags.
   - **Event handlers and ISRs that fire after `draw_dashboard(true)` must not use
     `ESP_LOGW`, `ESP_LOGE`, or any `ESP_LOG*` macro — even for error conditions.** Log
     suppression is permanent once activated. Instead: set a status variable
     (e.g. `s_phase = PHASE_ERROR`) and populate an error reason buffer before signaling
     the main task. The main task reflects the error in the dashboard Status field.

     Safe for callbacks/handlers to write after suppression:
     - `volatile` enum or bool status flags
     - Fixed-length char buffers, provided they are written *before* the synchronization
       primitive is signaled (the signal acts as a memory barrier for the reader)
     - `xEventGroupSetBits()`, `xTaskNotifyGive()`, `xSemaphoreGiveFromISR()`

     Unsafe after suppression: `esp_rom_printf()`, any `ESP_LOG*` macro, `printf`, `fprintf`.

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

## Pattern B Workflow — Per-Cycle Snapshot (cursor-home + overwrite, timer-driven)

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

### Progress bar computation (time-based, for duty-cycled windows)

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

## Pattern B-Lifecycle Workflow — Sequential Pipeline (lifecycle-driven)

Use for always-on firmware that progresses through discrete sequential phases with a fixed
field set. No deep sleep. No continuous event stream. No periodic `esp_timer`.

Examples: OTA firmware update pipeline, Wi-Fi provisioning wizard, factory test sequence,
boot health check.

### draw_dashboard(bool initial) — same signature as Pattern B

Identical structure to Pattern B: `initial = true` clears screen and suppresses logs;
all calls do `ANSI_HOME` + overwrite each row + `ANSI_ERASE_EOL`. The difference is in
*when* and *how often* it is called — lifecycle events rather than a timer.

### Module-scope state

Use a status enum covering all pipeline phases, plus supporting fields:

```c
typedef enum {
    PHASE_BOOTING,
    PHASE_CONNECTING,     /* or any phase name matching your pipeline */
    PHASE_READY,
    PHASE_WORKING,
    PHASE_COMPLETE,
    PHASE_ERROR,
} phase_t;

static volatile phase_t s_phase        = PHASE_BOOTING;
static char   s_error_reason[64]       = "";   /* set before entering PHASE_ERROR */
static int    s_elapsed_ms             = 0;    /* for polling loop elapsed time display */
```

### Refresh strategy: lifecycle-driven redraws

Each phase calls `draw_dashboard(false)` when state changes. For blocking operations that
must show live progress, use one of two sub-patterns:

#### Sub-pattern 1: Polling loop substitution

Replaces a single blocking wait (event group, semaphore, task notification) with a loop
of short-timeout iterations. Each iteration that times out (no event yet) updates elapsed
time and redraws the dashboard:

```c
/* Replace: xEventGroupWaitBits(group, BITS, F, F, pdMS_TO_TICKS(TIMEOUT_MS)) */
/* With: */
s_phase      = PHASE_CONNECTING;
s_elapsed_ms = 0;
draw_dashboard(false);

int         total_ms = 0;
EventBits_t bits     = 0;
while (total_ms < TIMEOUT_MS) {
    bits = xEventGroupWaitBits(group, BITS, pdFALSE, pdFALSE, pdMS_TO_TICKS(500));
    if (bits & TARGET_BITS) break;
    s_elapsed_ms += 500;
    total_ms     += 500;
    draw_dashboard(false);   /* dashboard shows "CONNECTING (4 s)" updating each tick */
}
```

Use when: a blocking wait can be split into short iterations without changing protocol
semantics (Wi-Fi association, HTTP response wait, button press with timeout).

The event handler that sets the bits must not call `draw_dashboard()` — it only calls
`xEventGroupSetBits()` and writes to state variables (see output discipline rule in step 6).

#### Sub-pattern 2: Threshold-based redraws

Inside a tight inner loop that already runs continuously (HTTPS chunked download, audio
decode, etc.), gate `draw_dashboard(false)` on a minimum progress increment to avoid
saturating the UART:

```c
int last_drawn = -1024;  /* negative initial value forces first draw immediately */
while (inner_loop_running) {
    /* perform one unit of work */
    s_bytes_read = get_current_progress();
    if (s_bytes_read - last_drawn >= 1024) {
        draw_dashboard(false);
        last_drawn = s_bytes_read;
    }
}
```

Use when: the inner loop runs faster than the UART can render frames (> ~4 frames/second
at 115200 baud). One dashboard refresh per 1 KB of progress is a reasonable default for
HTTPS OTA; tune the threshold based on typical transfer speeds and image size.

### Progress bar computation (byte-count-based, for download/transfer phases)

```c
/* Module-scope state */
static int s_total_bytes    = 0;   /* from Content-Length or equivalent; 0 if unknown */
static int s_received_bytes = 0;   /* updated in the inner loop */

/* In draw_dashboard(), progress row: */
if (s_phase == PHASE_WORKING && s_total_bytes > 0) {
    int pct  = (s_received_bytes * 100) / s_total_bytes;
    int fill = pct / 5;   /* 20-char bar: 1 char per 5 % */
    char bar[21];
    for (int i = 0; i < 20; i++) bar[i] = (i < fill) ? '=' : ' ';
    bar[20] = '\0';
    snprintf(buf, sizeof(buf),
             C_DIM "Progress:" C_RESET " [%s] %3d%%  %d / %d B" ANSI_ERASE_EOL "\r\n",
             bar, pct, s_received_bytes, s_total_bytes);
} else if (s_phase == PHASE_WORKING) {
    /* Total size unknown — show bytes received without percentage */
    snprintf(buf, sizeof(buf),
             C_DIM "Progress:" C_RESET " %d B received" ANSI_ERASE_EOL "\r\n",
             s_received_bytes);
} else {
    snprintf(buf, sizeof(buf),
             C_DIM "Progress:" C_RESET " \xe2\x80\x94" ANSI_ERASE_EOL "\r\n");
}
esp_rom_printf("%s", buf);
```

### Pre-populating state before draw_dashboard(true)

If the dashboard's first frame must show information that is only available after hardware
init (e.g. which OTA partition is active, a hardware version string), populate those state
variables *before* calling `draw_dashboard(true)`. `draw_dashboard(true)` reads whatever
is in the state variables when it renders the first frame — order matters:

```c
/* Correct order: */
led_set_status();        /* sets s_partition_name = "ota_0" */
draw_dashboard(true);    /* first frame shows Partition: ota_0 correctly */

/* Wrong order: */
draw_dashboard(true);    /* first frame shows Partition: unknown */
led_set_status();        /* too late — dashboard already drew */
```

### app_main structure (Pattern B-Lifecycle)

```c
void app_main(void) {
    /* Phase 0: hardware init — ESP_LOGx still active */
    hw_init();
    rollback_check();       /* or any pre-dashboard logic; use ESP_LOGI freely here */
    populate_state();       /* set s_partition_name, s_version, etc. */

    draw_dashboard(true);   /* clears screen, suppresses logs — ESP_LOGx disabled from here */

    /* Phase 1: blocking operation with polling loop substitution */
    if (blocking_connect() != OK) {
        /* s_phase already set to PHASE_ERROR inside blocking_connect() */
        while (true) { error_pattern(); vTaskDelay(pdMS_TO_TICKS(2000)); }
    }

    s_phase = PHASE_READY;
    draw_dashboard(false);

    /* Phase 2: wait for trigger */
    xSemaphoreTake(s_trigger_sem, portMAX_DELAY);

    /* Phase 3: long operation with threshold-based redraws */
    s_phase = PHASE_WORKING;
    draw_dashboard(false);
    esp_err_t err = run_long_operation();   /* calls draw_dashboard(false) internally */

    /* Phase 4: final state */
    if (err == ESP_OK) {
        s_phase = PHASE_COMPLETE;
        draw_dashboard(false);
        esp_restart();
    }
    snprintf(s_error_reason, sizeof(s_error_reason), "%s", esp_err_to_name(err));
    s_phase = PHASE_ERROR;
    draw_dashboard(false);
    while (true) { error_pattern(); vTaskDelay(pdMS_TO_TICKS(2000)); }
}
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
