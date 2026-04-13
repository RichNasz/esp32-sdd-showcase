<!-- ================================================
     AGENT-GENERATED — DO NOT EDIT BY HAND
     Generated from specs/ using esp32-sdd-documentation-generator skill
     Date: 2026-04-12 | Agent: Claude Code
     ================================================ -->

# Secure OTA via HTTPS — Button-Triggered Partition Flip Demo

> **Production OTA pattern.** Visual, button-triggered firmware update over HTTPS with TLS certificate validation, dual-partition rollback, and a clear LED indication of which image is running.

## Overview

The device maintains two firmware images in flash (ota_0 and ota_1). Pressing the button
triggers an HTTPS download from a fixed server URL. The new binary is written to the inactive
OTA partition. On success the device reboots into the new image and lights the opposite status
LED — left for ota_0, right for ota_1. A middle LED blinks during the transfer and shows a rapid
triple-blink pattern on failure. Rollback is enabled: if the new image does not call
`esp_ota_mark_app_valid_cancel_rollback()` on startup, the bootloader automatically reverts to
the previous image on the next boot.

## Board

| Item | Detail |
|---|---|
| Board | YEJMKJ ESP32-S3-DevKitC-1-N16R8 (16 MB flash, native USB OTG) |
| Target chip | ESP32-S3 |
| ESP-IDF | v5.5.x |
| USB port | Single USB-C port — `/dev/cu.usbmodem2101` for both flash and monitor |

## Hardware Wiring

| Signal | GPIO | Notes |
|---|---|---|
| Left LED — Program 1 / ota_0 | 4 | Active-high; steady ON when ota_0 is running |
| Middle LED — OTA / Error | 5 | Blinks during transfer; rapid triple-blink on failure |
| Right LED — Program 2 / ota_1 | 6 | Active-high; steady ON when ota_1 is running |
| OTA trigger button | 7 | Connect to GND when pressed; 3.3 V internal pull-up |

Use ~220–330 Ω current-limiting resistors on each LED.

## Build & Flash

```sh
cd examples/secure-ota-https

# The default OTA server URL (GitHub raw content) and embedded CA cert (ISRG Root X1)
# work out of the box. You only need to set your Wi-Fi credentials:
idf.py set-target esp32s3
idf.py menuconfig   # OTA Configuration → set Wi-Fi SSID and Password

# Build, flash, and open serial monitor (single port for both):
idf.py -p /dev/cu.usbmodem2101 flash monitor
```

If the device does not auto-reset for flashing, hold **BOOT**, press **RESET**,
release **BOOT**, then re-run the command.

### Setting Wi-Fi Credentials

Run menuconfig and fill in your network details under **OTA Configuration**:

```sh
idf.py menuconfig   # navigate to: OTA Configuration → Wi-Fi SSID / Wi-Fi Password
```

menuconfig writes the values you enter to a file called `sdkconfig` in the example
folder. That file is listed in `.gitignore` and is **never committed to git** — your
real credentials stay on your machine only.

**Why this is safe for a public repository:**

- `sdkconfig.defaults` (the file that _is_ committed) contains only the placeholder
  strings `"myssid"` and `"mypassword"`. It exists to document the available options,
  not to hold real values.
- `sdkconfig` (where menuconfig writes real values) is gitignored. It cannot be
  accidentally pushed.
- The build system merges the two at build time: `sdkconfig.defaults` provides
  factory defaults; `sdkconfig` overrides them with your local values.

**The one rule:** never put real credentials in `sdkconfig.defaults`. That file is in
git. `sdkconfig` is not.

**Optional — local override file (avoids re-entering after `idf.py fullclean`):**

Create `sdkconfig.defaults.local` alongside `sdkconfig.defaults`:

```
CONFIG_OTA_WIFI_SSID="MyRealNetwork"
CONFIG_OTA_WIFI_PASSWORD="MyRealPassword"
```

Then build with:

```sh
idf.py -DSDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.defaults.local" build
```

Add `sdkconfig.defaults.local` to your personal global git exclude
(`~/.config/git/ignore`) so it is never committed regardless of which project you
are in.

**General principle:** this gitignored-sdkconfig pattern is the standard ESP-IDF
approach for keeping secrets out of public repositories. It applies to any Kconfig
symbol that holds a secret: API keys, MQTT passwords, device tokens, and so on.
If a symbol's default value would expose a secret on GitHub, it belongs in
`sdkconfig` (via menuconfig or a gitignored local defaults file) — not in
`sdkconfig.defaults`.

### OTA Server Certificate

The default `main/server_cert.pem` contains the **ISRG Root X1** root CA. This validates
the default OTA server (GitHub raw content, which uses Let's Encrypt). No certificate change
is needed for the default configuration.

To use a different OTA server, replace `main/server_cert.pem` with the CA certificate that
signed your server's TLS certificate, and update `CONFIG_OTA_SERVER_URL` via menuconfig.

### Opening in VS Code / Cursor

Open **only this folder** (not the repository root) so the ESP-IDF Extension detects the
project: **File → Open Folder… → select `examples/secure-ota-https/`**.

## Expected Serial Output

Early boot (before dashboard initialises — plain ESP-IDF logs):

```
I (312) ota_main: Rollback check: OTA image state = 3 (already confirmed)
I (315) ota_main: Rollback check: app marked valid
I (317) ota_main: Running from ota_0 — left LED ON
```

After `draw_dashboard(true)` clears the screen and suppresses logs, the terminal
shows a fixed 7-row dashboard that refreshes in-place throughout the OTA lifecycle:

```
  OTA MONITOR   ESP32-SDD
Board: YEJMKJ ESP32-S3-DevKitC-1-N16R8   Partition: ota_0
Server: https://raw.githubusercontent.com/RichNasz/esp32-sdd-showcase/...
Wi-Fi:  CONNECTING (4 s)
Status: WIFI_CONNECTING
Progress: —
----------------------------------------
```

Once Wi-Fi connects and the button is pressed:

```
  OTA MONITOR   ESP32-SDD
Board: YEJMKJ ESP32-S3-DevKitC-1-N16R8   Partition: ota_0
Server: https://raw.githubusercontent.com/RichNasz/esp32-sdd-showcase/...
Wi-Fi:  CONNECTED (192.168.1.42)
Status: DOWNLOADING
Progress: [============        ]  61%  545259 / 892416 B
----------------------------------------
```

On success, the final dashboard state before reboot:

```
  OTA MONITOR   ESP32-SDD
Board: YEJMKJ ESP32-S3-DevKitC-1-N16R8   Partition: ota_0
Server: https://raw.githubusercontent.com/RichNasz/esp32-sdd-showcase/...
Wi-Fi:  CONNECTED (192.168.1.42)
Status: COMPLETE
Progress: [====================] 100%  892416 / 892416 B
----------------------------------------
```

After reboot, early boot logs again (dashboard not yet active):

```
I (312) ota_main: Rollback check: OTA image state = 2 (PENDING_VERIFY — marking valid now)
I (315) ota_main: Rollback check: app marked valid
I (317) ota_main: Running from ota_1 — right LED ON
```

## Key Concepts

- `esp_https_ota_begin` / `esp_https_ota_perform` / `esp_https_ota_finish` — advanced OTA
  API for per-chunk progress updates; each `perform` call returns
  `ESP_ERR_HTTPS_OTA_IN_PROGRESS` until the transfer is complete
- `esp_ota_mark_app_valid_cancel_rollback()` — called unconditionally at startup before any
  user logic; prevents the bootloader from rolling back on the next reboot
- `esp_ota_get_running_partition()` + `esp_ota_get_state_partition()` — detect which OTA
  slot is active and whether a rollback verification is pending
- Embedded server CA certificate via `target_add_binary_data` in `main/CMakeLists.txt`
- Dual OTA partition table (`partitions.csv`) with two 1.5 MB app slots
- `CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=y` in `sdkconfig.defaults`
- ANSI TUI dashboard — 7-row fixed-layout cursor-home+overwrite pattern; `draw_dashboard(true)`
  clears the screen, hides the cursor, and permanently suppresses ESP-IDF logs;
  all subsequent output is via `esp_rom_printf()` only
- Wi-Fi polling loop — replaces a single blocking `xEventGroupWaitBits` with 500 ms iterations
  so the dashboard can show elapsed connect time each tick
- Separate FreeRTOS task for LED blinking because `app_main` blocks during the OTA transfer
- Binary semaphore driven from GPIO ISR for button edge detection without polling

## Testing

Testing philosophy: automated first, manual only when hardware observation is unavoidable.

### Automated Tests

1. **T-A1 — Zero-Warning Build**

   ```sh
   cd examples/secure-ota-https
   idf.py set-target esp32s3 && idf.py build
   ```

   Pass: exit code 0, zero compiler warnings.

2. **T-A2 — Partition Table Validation**

   ```sh
   idf.py partition-table 2>&1 | grep -E "ota_0|ota_1"
   ```

   Pass: both `ota_0` and `ota_1` partitions listed with ≥ 1536K (1.5 MB) each.

3. **T-A3 — Rollback Config Present**

   ```sh
   grep "CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=y" examples/secure-ota-https/sdkconfig.defaults
   ```

   Pass: line present.

4. **T-A4 — Server Cert Embedded in Binary**

   ```sh
   idf.py build 2>&1 | grep "server_cert"
   ```

   Pass: `server_cert.pem` appears in build output (confirms `target_add_binary_data` is working).

### Manual Tests — hardware required

**T-M1 — Full OTA Download and Flash (default GitHub server)**

Why manual: requires a live Wi-Fi network and real target hardware.

The default configuration uses GitHub raw content as the OTA server and ISRG Root X1 as the
embedded CA. No local server setup is needed.

1. Set `OTA_WIFI_SSID` and `OTA_WIFI_PASSWORD` via `idf.py menuconfig`.
2. Build and flash: `idf.py -p /dev/cu.usbmodem2101 flash monitor`
3. Confirm serial log: Wi-Fi connected → "Ready — press button to trigger OTA".
4. Press the trigger button (GPIO 7).
5. Confirm progress log: downloading → writing → rebooting.
6. Confirm device reboots; opposite status LED lights up; log shows "app marked valid".

Pass: OTA completes; device boots new firmware; rollback not triggered.

---

**T-M2 — TLS Certificate Rejection (Invalid Cert)**

Why manual: negative TLS test requires a deliberate certificate mismatch on real hardware.

1. Replace `main/server_cert.pem` with a certificate from a different CA.
2. Build and flash.
3. Press trigger button and confirm serial log shows TLS handshake failure.
4. Confirm the device does not flash the new binary (middle LED shows error pattern; no reboot).

Pass: TLS error logged; no reboot; current firmware intact.

---

**T-M3 — Automatic Rollback**

Why manual: rollback requires a firmware image that intentionally fails to call
`mark_app_valid`, which cannot be simulated without a second custom-built binary.

1. Build a "bad" firmware variant that omits `esp_ota_mark_app_valid_cancel_rollback()`.
2. OTA-flash the bad firmware via the T-M1 procedure.
3. Confirm bootloader rolls back to the previous image on the next boot.
4. Confirm the correct LED re-illuminates and the serial log shows the rollback cause.

Pass: device rolls back automatically; previous firmware resumes.

## Spec Files

- [specs/FunctionalSpec.md](specs/FunctionalSpec.md)
- [specs/CodingSpec.md](specs/CodingSpec.md)
- [specs/TestSpec.md](specs/TestSpec.md)
