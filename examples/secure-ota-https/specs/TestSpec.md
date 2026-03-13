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

### T-M1 — Full OTA Download and Flash

**Why manual**: Requires a live HTTPS server with a self-signed certificate and a real Wi-Fi network.

Prerequisites:
- `openssl req -x509 -newkey rsa:2048 -keyout key.pem -out cert.pem -days 365 -nodes -subj "/CN=localhost"`
- `python3 -m http.server 8070 --bind 0.0.0.0` (or a simple HTTPS server)
- Place a valid `firmware.bin` at the server root.

Steps:
1. Copy `cert.pem` to `main/server_cert.pem` in the example.
2. Set `CONFIG_OTA_SERVER_URL`, `CONFIG_WIFI_SSID`, `CONFIG_WIFI_PASSWORD` in sdkconfig.
3. Build and flash: `idf.py build flash`
4. Open monitor: `idf.py monitor`
5. Confirm progress log: connecting → fetching → writing → rebooting.
6. Confirm device reboots into new firmware and logs "Marked valid".

Pass: OTA completes; device boots new firmware; rollback not triggered.

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
