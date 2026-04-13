# ==================== BOARD SELECTION ====================
# Change only this section, then regenerate the project
#
# Board: YEJMKJ ESP32-S3-DevKitC-1-N16R8          ← ACTIVE / DEFAULT
#
# =======================================================
# Institutional Memory: shared-specs/AIGenLessonsLearned.md — read before regenerating

# Coding Specification — Secure OTA via HTTPS

## Architecture

Linear OTA pipeline. App_main performs Wi-Fi association, immediately checks for a pending
rollback and validates the running image if needed, then initiates the HTTPS OTA download.
A separate FreeRTOS task drives the LED blink during the download so app_main can block on the
transfer without losing visual feedback. On success, app_main reboots. On any failure, the
pipeline halts with a visible error LED pattern and does not reboot silently.

## Key Constraints

- Server certificate is embedded at build time from main/server_cert.pem using
  target_add_binary_data in CMakeLists. This file is a placeholder — operators must replace it
  with their actual CA certificate before use.
- Wi-Fi association timeout: 30 seconds before the pipeline is abandoned.
- HTTPS client timeout: 30 seconds to avoid indefinite blocking on a stalled server connection.
- Partition table: dual OTA layout (ota_0 + ota_1 + otadata) defined in a custom partitions.csv
  at the example root. The default ESP-IDF partition table is too small for two OTA app slots.
- Rollback: CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=y must be set. The newly-booted image must
  call esp_ota_mark_app_valid_cancel_rollback() early in app_main — before any user logic that
  could fail. Failure to do so causes the bootloader to roll back on the next reboot.
- All configurable parameters (Wi-Fi credentials, OTA server URL) must be exposed via Kconfig,
  not hardcoded.

## Preferred Libraries and APIs

Use esp_https_ota for the OTA download. It handles chunked writes to the OTA flash partition,
HTTPS negotiation, and progress callbacks in a single API call. Implementing this manually via
esp_ota_write would duplicate logic that esp_https_ota already handles correctly.

Use standard STA Wi-Fi with event loop for network connectivity — the same pattern as the
deep-sleep-bme280-mqtt-sensor example. No reconnect or roaming logic is needed.

## Hardware Interface

| Signal         | GPIO | Direction | Notes                                      |
|----------------|------|-----------|--------------------------------------------|
| LED Left       | 4    | Output    | Active-high, 3.3 V drive; steady ON = ota_0 active |
| LED Middle     | 5    | Output    | Active-high; blinks during OTA, error pattern on failure |
| LED Right      | 6    | Output    | Active-high; steady ON = ota_1 active      |
| Button Trigger | 7    | Input     | Active-low; ESP32-S3 internal pull-up enabled |

## Non-Functional Requirements

- OTA API failures must not trigger abort(). Log the error with esp_err_to_name, blink the
  error LED pattern, and then loop without rebooting. Silent reboots on persistent OTA failure
  would create an infinite retry loop if the server is unavailable.
- Wi-Fi connection timeout must log a clear message and halt execution — do not reboot silently.
  The operator must be able to observe the failure cause from the serial log.
- The LED blink during download must run on a separate FreeRTOS task, because app_main blocks
  on the esp_https_ota call for the full transfer duration.
- All state transitions must be visible to the operator. Before the ANSI dashboard is
  initialised (early boot, rollback check) use ESP_LOGx. After `draw_dashboard(true)` clears the
  screen and suppresses logging, all diagnostic output is provided by the dashboard Status field.
  Never use ESP_LOGx after log suppression begins — reflect errors in the Status field instead.

## Gotchas

- If rollback is enabled but esp_ota_mark_app_valid_cancel_rollback() is never called, the
  device will roll back on every subsequent reboot even after a successful OTA. This call must
  be unconditional and must happen before any user logic.
- The PEM file embedded in the binary must be a CA certificate that can validate the OTA
  server's TLS chain. The default OTA URL (GitHub raw content) uses Let's Encrypt, which
  chains to ISRG Root X1 — embed that root CA (see main/server_cert.pem). For a
  self-signed test server, embed the server's own certificate directly instead.
- Custom partition tables require CONFIG_PARTITION_TABLE_CUSTOM=y and
  CONFIG_PARTITION_TABLE_CUSTOM_FILENAME pointing to the CSV in sdkconfig.defaults.
- The YEJMKJ board uses native USB only (no CH343P bridge). Its single USB-C connector
  enumerates as a single port (`/dev/cu.usbmodem2101`). Both flash and monitor use the
  same port: `idf.py -p /dev/cu.usbmodem2101 flash monitor`. Set
  CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y in sdkconfig.defaults. Do NOT set
  CONFIG_ESP_CONSOLE_USB_CDC=y.

## Monitor Dashboard

### Pattern choice

Fixed-layout cursor-home + overwrite (Pattern B semantics from the `esp32-ansi-monitor-engineer`
skill, adapted for always-on firmware). The OTA pipeline progresses through discrete sequential
phases — no continuous scrolling event stream. A DECSTBM scroll region (Pattern A) would add
complexity without benefit; cursor-home + overwrite produces a clean, readable fixed layout.

### Dashboard layout — 7 rows

```
Row 1: title bar — "  OTA MONITOR   ESP32-SDD  " (bold, blue background)
Row 2: Board: YEJMKJ ESP32-S3-DevKitC-1-N16R8   Partition: ota_0 / ota_1
Row 3: Server: <OTA server URL, truncated to ~60 chars>
Row 4: Wi-Fi: CONNECTING (N s) / CONNECTED (x.x.x.x) / FAILED
Row 5: Status: BOOTING / READY — press button / DOWNLOADING / COMPLETE / ERROR: <reason>
Row 6: Progress: [====================] 100%  47328 / 47328 B  (shown as "—" when not downloading)
Row 7: separator line
```

### Status state machine

- `BOOTING` — from `draw_dashboard(true)` until Wi-Fi connection attempt starts
- `WIFI_CONNECTING` — while the Wi-Fi polling loop is running
- `WIFI_FAILED` — if Wi-Fi times out or is rejected (shown in error loop)
- `READY` — after Wi-Fi connects; waiting for button press
- `DOWNLOADING` — from `run_ota()` start until it returns
- `COMPLETE` — after `run_ota()` succeeds; drawn once before `esp_restart()`
- `ERROR: <reason>` — on any failure; shown in the error loop

### Refresh architecture

No periodic `esp_timer` is needed — the OTA lifecycle drives redraws:

1. **Boot phase**: Call `draw_dashboard(true)` once after `led_set_status()` and the rollback
   check, before `wifi_connect()`. This clears the screen, hides the cursor, suppresses
   ESP-IDF logs permanently, and shows `Status: BOOTING`.
2. **Wi-Fi phase**: Replace the single blocking `xEventGroupWaitBits(…, WIFI_CONNECT_TIMEOUT_MS)`
   with a polling loop using a 500 ms timeout per iteration. On each tick, increment
   `s_wifi_elapsed_ms` by 500 and call `draw_dashboard(false)` to show elapsed time in Row 4.
   On connection or failure, update `s_ota_status` and call `draw_dashboard(false)` once.
3. **Ready phase**: Set `s_ota_status = OTA_STATUS_READY` and call `draw_dashboard(false)` once
   before `xSemaphoreTake(s_button_sem, portMAX_DELAY)`.
4. **Download phase**: Inside the `esp_https_ota_perform()` loop, update `s_bytes_read` and
   `s_img_size` and call `draw_dashboard(false)` on each iteration where bytes read changes
   (gate on ≥ 1 KB change to avoid excessive UART traffic on fast connections).
5. **Final phase**: Set the terminal status, call `draw_dashboard(false)` once, then
   `esp_restart()` (success) or enter the `led_error_pattern()` loop (failure).

Only `app_main` and functions called from `app_main` may call `draw_dashboard()`. The Wi-Fi
event handler must not call `esp_rom_printf()` or `draw_dashboard()` — it sets event group bits
only.

### Output discipline

- `draw_dashboard(true)` must call `esp_log_level_set("*", ESP_LOG_NONE)` after the initial
  clear — permanent log suppression for the rest of the session.
- All terminal output after suppression via `esp_rom_printf()` exclusively — never `printf`,
  `fprintf`, or `ESP_LOG*`. Pre-format with `snprintf` into a stack buffer, then emit with
  `esp_rom_printf("%s", buf)`.
- Every row ends with `ANSI_ERASE_EOL` to clear stale trailing characters from prior draws.
- Write content first, then `ANSI_ERASE_EOL` — never blank the line first (causes visible flash
  at 115200 baud while new content fills in).
- Include `esp_rom_sys.h` and add `esp_rom` to `main/CMakeLists.txt REQUIRES`.

## Wi-Fi Credential Safety (README documentation requirement)

The README must include a dedicated section — "Setting Wi-Fi Credentials" — that explains
both the mechanism and the security rationale. The target audience is someone unfamiliar with
ESP-IDF who might otherwise edit sdkconfig.defaults or hardcode credentials in source.

The section must cover the following points, in plain language:

1. **How to set credentials** — run `idf.py menuconfig`, navigate to OTA Configuration,
   and fill in Wi-Fi SSID and Wi-Fi Password. menuconfig writes them to `sdkconfig`.

2. **Why this is safe for a public repo** — the `.gitignore` generated by this skill
   explicitly excludes `sdkconfig` and `sdkconfig.old`. Real credentials written by menuconfig
   never reach git. The `sdkconfig.defaults` checked into the repo contains only the
   placeholder strings `"myssid"` and `"mypassword"` — never real values.

3. **The rule to follow** — never put real network credentials in `sdkconfig.defaults`.
   That file is committed to git. `sdkconfig` is not.

4. **Optional: local override file** — for users who want to avoid re-entering credentials
   after `idf.py fullclean`, document the `sdkconfig.defaults.local` pattern:
   create a file named `sdkconfig.defaults.local` with the real values and build with:
   `idf.py -DSDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.defaults.local" build`.
   Instruct them to add `sdkconfig.defaults.local` to their personal `.gitignore` or
   global git exclude (`~/.config/git/ignore`) so it is never committed.

5. **General principle** — this gitignored-sdkconfig pattern is the standard ESP-IDF
   approach for keeping secrets out of public repositories. The same pattern applies to any
   Kconfig symbol that holds a secret: API keys, MQTT passwords, device tokens, etc.

## Firmware Update Push Workflow (README documentation requirement)

The README must include a dedicated section — "Pushing a Firmware Update" — that explains
the complete command-line workflow for building a new binary and making it available to
devices via OTA. The target audience is someone who has already flashed a device and now
wants to update the firmware the OTA server serves.

The section must cover the following steps, presented as a numbered command-line workflow:

1. **Increment the app version** — open `sdkconfig.defaults` and bump `CONFIG_APP_PROJECT_VER`
   (e.g. `"1.0.1"` → `"1.0.2"`). This makes the new build identifiable in the dashboard and
   in the rollback state machine. Commit this change alongside the binary so the version in
   the repo always matches the binary on disk.

2. **Set Wi-Fi credentials** — ensure `sdkconfig` has real credentials (either via
   `idf.py menuconfig` or a gitignored `sdkconfig.defaults.local`). The binary will be
   flashed to devices that use these credentials to connect. See the "Setting Wi-Fi
   Credentials" section for the safe approach.

3. **Delete stale sdkconfig so sdkconfig.defaults is re-applied** — when `CONFIG_APP_PROJECT_VER`
   changes in `sdkconfig.defaults`, the existing `sdkconfig` must be removed so the build
   system re-reads the defaults file. Without this step the version bump has no effect.
   Then rebuild:
   ```sh
   cd examples/secure-ota-https
   rm -f sdkconfig sdkconfig.old
   idf.py menuconfig   # re-enter Wi-Fi credentials after sdkconfig is deleted
   idf.py build
   ```

4. **Copy the binary to the example root** — the OTA server URL points to
   `examples/secure-ota-https/secure_ota_https.bin` in the repository root (tracked by git
   via the `!secure_ota_https.bin` exception in `.gitignore`):
   ```sh
   cp build/secure_ota_https.bin .
   ```

5. **Commit and push** — include both `sdkconfig.defaults` (version bump) and the binary:
   ```sh
   git add sdkconfig.defaults secure_ota_https.bin
   git commit -m "build(secure-ota-https): update OTA binary to v1.0.2"
   git push
   ```
   Within seconds of the push, GitHub raw content serves the new binary at the OTA URL.
   Any device that triggers OTA will download and flash this version.

6. **Verify** — open the GitHub raw URL in a browser and confirm the file size matches
   the build output. The OTA client on the device will validate the binary signature and
   flash integrity before rebooting.

The README must note that the binary is **not** a build artefact to be ignored — it is the
live OTA payload for this example and must be kept in sync with the source. Every version
bump in `sdkconfig.defaults` must be accompanied by a matching binary push.

## File Layout (non-standard files)

- main/Kconfig.projbuild — Wi-Fi SSID/password, OTA server URL
- main/server_cert.pem — placeholder PEM; operator replaces with real CA cert
- partitions.csv — dual OTA partition table at example root
