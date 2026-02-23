# Testing Specification — Hardware AES Secure Storage

## Testing Philosophy

Automated testing using native ESP-IDF tools is strongly preferred.
Additional tools are only allowed with explicit justification and documented benefits.
Manual testing is used only when no practical automated alternative exists.

---

## Automated Tests

### T-A1 — Zero-Warning Build

**Tool**: `idf.py` (native)

```sh
cd examples/hardware-aes-secure-storage
idf.py set-target esp32s3 && idf.py build
```

Pass: exit code 0, zero compiler warnings.

---

### T-A2 — Binary Size Check

**Tool**: `idf.py` (native)

```sh
idf.py size | grep "Total binary size"
```

Pass: < 1.5 MB.

---

### T-A3 — Unity Self-Test via Serial Log (Wokwi)

**Tool**: Wokwi CLI — justification: mbedTLS AES-CBC runs identically in simulation; the round-trip self-test result is fully verifiable from serial output.

Prerequisites: Wokwi CLI installed; `wokwi.toml` + `diagram.json` at example root.

```sh
wokwi-cli --timeout 15000 --expect "Self-test: PASS" .
```

Pass: `Self-test: PASS` appears in simulated serial output within 15 seconds.

---

### T-A4 — Hardware AES Enabled Confirmation

**Tool**: `idf.py` (native)

```sh
grep "CONFIG_MBEDTLS_HARDWARE_AES=y" examples/hardware-aes-secure-storage/sdkconfig.defaults
```

Pass: line present (hardware accelerator correctly configured).

---

## Manual Tests

### T-M1 — Self-Test on Real Hardware

**Why manual**: Confirms hardware AES accelerator is invoked (not software fallback) on actual ESP32-S3 silicon; Wokwi may use a software implementation.

1. Flash to XIAO ESP32S3: `idf.py set-target esp32s3 && idf.py build flash`
2. Open monitor: `idf.py monitor`
3. Confirm serial shows `Self-test: PASS` and LED blinks once.
4. Confirm throughput log shows ≥ 5 MB/s (hardware AES; software typically < 1 MB/s).

Pass: PASS logged, single LED blink, throughput ≥ 5 MB/s.

---

### T-M2 — NVS Persistence Across Resets

**Why manual**: NVS flash persistence requires physical flash write/read cycles.

1. Flash and run — confirm AES key is generated and stored ("generated, saved to NVS").
2. Reset the board (press EN button).
3. Confirm serial shows "loaded from NVS" (key persists across reset).
4. Confirm self-test still passes (decryption works with persisted key).

Pass: key loads from NVS after reset; self-test passes.
