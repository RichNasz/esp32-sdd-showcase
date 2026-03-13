<!-- ================================================
     AGENT-GENERATED — DO NOT EDIT BY HAND
     Generated from specs/ using esp32-sdd-full-project-generator skill
     Date: 2026-03-11 | Agent: Claude Code
     ================================================ -->

# Secure OTA via HTTPS — Button-Triggered Partition Flip Demo

## Overview

Minimal, visual demonstration of ESP32-S3 OTA using dual partitions. The device runs one of two
firmware images — "Program 1" (left LED on) or "Program 2" (right LED on). Pressing the button
triggers an HTTPS download from a fixed server URL; the new binary is written to the inactive OTA
partition; on success the device reboots into the new image and lights the opposite LED. A middle
LED blinks during the transfer and shows a rapid error pattern on failure. Rollback is enabled:
if the new image does not call `esp_ota_mark_app_valid_cancel_rollback()` on startup, the bootloader
automatically reverts to the previous image on the next boot.

## Prerequisites

| Item | Detail |
|---|---|
| ESP-IDF | v5.4.x or v5.5.x |
| Target chip | ESP32-S3 |
| Board | YEJMKJ ESP32-S3-DevKitC-1-N16R8 (16 MB flash, CH343P USB-to-UART) |
| External hardware | 3× LEDs + 220–330 Ω resistors on GPIO 4, 5, 6; push button (GPIO 7 → GND) |
| OTA server | Any HTTPS endpoint with a self-signed or CA-signed certificate |

### Hardware Wiring

| LED / Button | GPIO | Notes |
|---|---|---|
| Left LED (Program 1 / ota_0) | 4 | Active-high; steady ON when ota_0 is running |
| Middle LED (OTA / Error) | 5 | Blinks during transfer; rapid triple-blink on failure |
| Right LED (Program 2 / ota_1) | 6 | Active-high; steady ON when ota_1 is running |
| Trigger button | 7 | Connect to GND when pressed; 3.3 V internal pull-up |

## Build & Flash

```sh
cd examples/secure-ota-https

# 1. Replace the placeholder certificate with your actual server CA cert:
#    cp /path/to/server-ca.pem main/server_cert.pem

# 2. Set target and configure credentials:
idf.py set-target esp32s3
idf.py menuconfig   # OTA Configuration → Wi-Fi SSID, Password, Server URL

# 3. Build, flash, and monitor:
idf.py build flash monitor
```

Use the **USB-to-Serial USB-C port** (same side as the RST + BOOT buttons) for flashing.
The CH343P driver handles auto-reset on most systems. If auto-reset fails:
1. Hold **BOOT**, press **RST**, release **BOOT**, then run `idf.py flash monitor`.

### Opening in VS Code / Cursor

This folder is a complete, standalone ESP-IDF project. Open **only this folder**
(not the repository root) in VS Code or Cursor so the official ESP-IDF Extension
can detect and configure the project automatically.

**File → Open Folder… → select `examples/secure-ota-https/`**

This gives you one-click Build / Flash / Monitor / Debug, Menuconfig, and size
analysis from the ESP-IDF commands palette. Do not open the top-level repository
folder as your active workspace while developing this example.

## Expected Serial Output

```
I (312) ota_main: Rollback check: OTA image state = 3 (already confirmed)
I (315) ota_main: Rollback check: app marked valid
I (317) ota_main: Running from ota_0 — left LED ON
I (2401) ota_main: Wi-Fi connect: SSID=MyNetwork ...
I (4823) ota_main: Wi-Fi connected — IP: 192.168.1.42
I (4826) ota_main: Ready — press button (GPIO7) to trigger OTA from: https://192.168.1.100:8070/firmware.bin

--- (button pressed) ---

I (11044) ota_main: Button pressed — starting OTA download
I (11221) ota_main: OTA start — URL: https://192.168.1.100:8070/firmware.bin
I (11489) ota_main: OTA image size: 892416 bytes
I (12103) ota_main: OTA progress: 10% (89241 / 892416 bytes)
I (13891) ota_main: OTA progress: 30% (267724 / 892416 bytes)
I (16204) ota_main: OTA progress: 60% (535449 / 892416 bytes)
I (18819) ota_main: OTA progress: 90% (803174 / 892416 bytes)
I (19441) ota_main: OTA success — new image written to inactive partition
I (19442) ota_main: OTA complete — rebooting into new firmware

--- (reboot) ---

I (312) ota_main: Rollback check: OTA image state = 2 (PENDING_VERIFY — marking valid now)
I (315) ota_main: Rollback check: app marked valid
I (317) ota_main: Running from ota_1 — right LED ON
```

## Key Concepts

- `esp_https_ota_begin` / `esp_https_ota_perform` / `esp_https_ota_finish` — advanced OTA API
  for per-chunk progress logging while keeping the download in a single component
- `esp_ota_mark_app_valid_cancel_rollback()` — called unconditionally at startup before any
  user logic; prevents the bootloader from rolling back on the next reboot
- `esp_ota_get_running_partition()` + `esp_ota_get_state_partition()` — detect which OTA slot
  is active and whether a rollback verification is pending
- Embedded server certificate via `target_add_binary_data` in `main/CMakeLists.txt`
- Dual OTA partition table (`partitions.csv`) with two 1.5 MB app slots
- `CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=y` in `sdkconfig.defaults`
- Separate FreeRTOS task for LED blinking because `app_main` blocks during the OTA transfer
- Binary semaphore from GPIO ISR for button debounce-free edge detection

## Testing

### Automated Tests

**T-A1 — Zero-Warning Build**

```sh
cd examples/secure-ota-https
idf.py set-target esp32s3 && idf.py build
```

Pass: exit code 0, zero compiler warnings.

---

**T-A2 — Partition Table Validation**

```sh
idf.py partition-table 2>&1 | grep -E "ota_0|ota_1"
```

Pass: both `ota_0` and `ota_1` partitions listed with ≥ 1536K (1.5 MB) each.

---

**T-A3 — Rollback Config Present**

```sh
grep "CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=y" examples/secure-ota-https/sdkconfig.defaults
```

Pass: line present.

---

**T-A4 — Server Cert Embedded in Binary**

```sh
idf.py build 2>&1 | grep "server_cert"
```

Pass: `server_cert.pem` appears in build output (confirms `target_add_binary_data` is working).

---

### Manual Tests — hardware required

**T-M1 — Full OTA Download and Flash**

Why manual: requires a live HTTPS server with a self-signed certificate and a real Wi-Fi network.

Prerequisites:
```sh
openssl req -x509 -newkey rsa:2048 -keyout key.pem -out cert.pem \
  -days 365 -nodes -subj "/CN=ota-server"
# Serve firmware.bin over HTTPS on port 8070 (e.g. using caddy or openssl s_server)
```

Steps:
1. Copy `cert.pem` to `main/server_cert.pem`.
2. Set `CONFIG_OTA_SERVER_URL`, `CONFIG_OTA_WIFI_SSID`, `CONFIG_OTA_WIFI_PASSWORD` via `idf.py menuconfig`.
3. Build and flash: `idf.py build flash`
4. Open monitor: `idf.py monitor`
5. Press the trigger button (GPIO7).
6. Confirm progress log: connecting → fetching → writing → rebooting.
7. Confirm device reboots and lights the opposite status LED.

Pass: OTA completes; device boots new firmware; rollback not triggered; correct LED illuminated.

---

**T-M2 — TLS Certificate Rejection (Invalid Cert)**

Why manual: negative TLS test requires a deliberate certificate mismatch on real hardware.

1. Replace `main/server_cert.pem` with a certificate from a *different* CA.
2. Build and flash.
3. Press trigger button and confirm serial log shows TLS handshake failure / certificate verify error.
4. Confirm the device does not flash the new binary (middle LED shows error pattern; no reboot).

Pass: TLS error logged; no reboot; current firmware intact.

---

**T-M3 — Automatic Rollback**

Why manual: rollback requires a firmware image that intentionally fails to call `mark_app_valid`,
which cannot be simulated without a second custom-built binary.

1. Build a "bad" firmware variant that does not call `esp_ota_mark_app_valid_cancel_rollback()`.
2. OTA-flash the bad firmware via the T-M1 procedure.
3. Confirm bootloader rolls back to the previous image on the next boot.
4. Confirm serial log shows the rollback boot cause and the correct LED re-illuminates.

Pass: device rolls back automatically; previous firmware resumes.

## Spec Files

- [specs/FunctionalSpec.md](specs/FunctionalSpec.md)
- [specs/CodingSpec.md](specs/CodingSpec.md)
- [specs/TestSpec.md](specs/TestSpec.md)
