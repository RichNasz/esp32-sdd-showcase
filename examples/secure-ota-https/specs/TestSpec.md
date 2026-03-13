# Testing Specification — Secure OTA via HTTPS

## Testing Philosophy

Automated testing using native ESP-IDF tools is strongly preferred.
Additional tools are only allowed with explicit justification and documented benefits.
Manual testing is used only when no practical automated alternative exists.

---

## Automated Tests

### T-A1 — Zero-Warning Build

**Tool**: `idf.py` (native)

```sh
cd examples/secure-ota-https
idf.py set-target esp32s3 && idf.py build
```

Pass: exit code 0, zero compiler warnings.

---

### T-A2 — Partition Table Validation

**Tool**: `idf.py` (native)

```sh
idf.py partition-table 2>&1 | grep -E "ota_0|ota_1"
```

Pass: both `ota_0` and `ota_1` partitions listed with ≥ 1536K (1.5 MB) each.

---

### T-A3 — Rollback Config Present

**Tool**: `idf.py` (native — config check)

```sh
grep "CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=y" examples/secure-ota-https/sdkconfig.defaults
```

Pass: line present.

---

### T-A4 — Server Cert Embedded in Binary

**Tool**: `idf.py` (native)

```sh
idf.py build 2>&1 | grep "server_cert"
```

Pass: `server_cert.pem` appears in build output (confirms `target_add_binary_data` is working).

---

## Manual Tests

*End-to-end OTA requires a real HTTPS server, Wi-Fi AP, and target hardware. These tests cannot be reliably automated without a complex multi-process CI rig.*

### T-M1 — Full OTA Download and Flash (default GitHub server)

**Why manual**: Requires a live Wi-Fi network and real target hardware.

The default configuration uses GitHub raw content as the OTA server
(`CONFIG_OTA_SERVER_URL` default) and ISRG Root X1 as the embedded CA
(`main/server_cert.pem`). No local server setup is needed for this path.

Prerequisites:
- A built firmware binary pushed to the repository at the URL in `CONFIG_OTA_SERVER_URL`.
- Wi-Fi credentials configured via Kconfig (`idf.py menuconfig → OTA Configuration`).

Steps:
1. Set `OTA_WIFI_SSID` and `OTA_WIFI_PASSWORD` via `idf.py menuconfig`.
2. Build and flash: `idf.py -p /dev/cu.usbmodem2101 flash monitor`
3. Confirm serial log: Wi-Fi connected → "Ready — press button to trigger OTA".
4. Press the button (GPIO 7).
5. Confirm progress log: downloading → writing → rebooting.
6. Confirm device reboots into new firmware; opposite status LED lights up; log shows "app marked valid".

Pass: OTA completes; device boots new firmware; rollback not triggered.

**Alternative: Local self-signed server**

For testing cert rejection or non-GitHub servers:
- `openssl req -x509 -newkey rsa:2048 -keyout key.pem -out cert.pem -days 365 -nodes -subj "/CN=localhost"`
- Serve the binary over HTTPS; place `cert.pem` in `main/server_cert.pem`; rebuild.

---

### T-M2 — TLS Certificate Rejection (Invalid Cert)

**Why manual**: Negative TLS test requires a deliberate certificate mismatch on real hardware.

1. Replace `main/server_cert.pem` with a certificate from a *different* CA.
2. Build and flash.
3. Confirm serial log shows TLS handshake failure / certificate verify error.
4. Confirm device does not flash the new binary (stays on current firmware).

Pass: TLS error logged; no reboot; current firmware intact.

---

### T-M3 — Automatic Rollback

**Why manual**: Rollback requires a firmware image that intentionally fails to call `mark_app_valid`, which cannot be simulated without a second custom-built binary.

1. Build a "bad" firmware variant that does not call `esp_ota_mark_app_valid_cancel_rollback()`.
2. OTA-flash the bad firmware via T-M1 procedure.
3. Confirm bootloader rolls back to the previous image on the next boot.
4. Confirm serial log shows the rollback boot cause.

Pass: device rolls back automatically; previous firmware resumes.
