# ==================== BOARD SELECTION ====================
# Change only this section, then regenerate the project
#
# Board: seeed-xiao-esp32s3          ← ACTIVE / DEFAULT
#
# =======================================================
# Institutional Memory: shared-specs/AIGenLessonsLearned.md — read before regenerating

# Coding Specification — Hardware AES Secure Storage

## Hardware AES Configuration

- Enable hardware AES via sdkconfig.defaults: `CONFIG_MBEDTLS_HARDWARE_AES=y`
- Use mbedTLS `mbedtls_aes_context` for all AES operations.
- Algorithm: AES-256-CBC.
- Key size: 32 bytes (256 bits).
- IV size: 16 bytes; generate fresh IV with `esp_fill_random()` on every `secure_write` call.
- Store IV prepended to ciphertext in NVS (format: [16 bytes IV][N bytes ciphertext]).
- Padding: PKCS#7 — pad plaintext to next 16-byte boundary before encryption.

## API Design

```c
/* secure_storage.h */
esp_err_t secure_storage_init(void);          // init NVS, load or generate AES key
esp_err_t secure_write(const char *key, const void *data, size_t len);
esp_err_t secure_read(const char *key, void *data, size_t *len);
```

- Implement in `main/secure_storage.c` + `main/secure_storage.h`.
- `secure_storage_init`: open NVS namespace `"sec_store"`; load key from key `"aes_key"`, or generate and save if absent.
- `secure_write`: encrypt data with fresh IV; write IV+ciphertext blob to NVS under `key`.
- `secure_read`: read blob from NVS; split IV from ciphertext; decrypt; write to caller buffer.
- Return `ESP_ERR_NOT_FOUND` if key does not exist in NVS.
- Return `ESP_ERR_INVALID_SIZE` if caller buffer is too small.

## NVS Layout

- Namespace: `"sec_store"`
- Key `"aes_key"`: 32-byte AES key (blob).
- Key `<user key>`: IV+ciphertext blob (blob, variable length).

## Self-Test (app_main)

1. Call `secure_storage_init()`.
2. `secure_write("test_key", "Hello, SDD!", 11)`.
3. `secure_read("test_key", buf, &len)`.
4. `memcmp(buf, "Hello, SDD!", 11)` — PASS or FAIL.
5. Log result; blink LED once on PASS, three times on FAIL.

## Throughput Benchmark

- Allocate 64 KB buffer on heap; fill with `esp_fill_random()`.
- Record `esp_timer_get_time()` before and after encrypting 64 KB with `mbedtls_aes_crypt_cbc`.
- Log: `ESP_LOGI(TAG, "HW AES-256-CBC: %.2f MB/s", throughput_mbs)`.

## LED Feedback

- GPIO 21, active LOW.
- PASS: 1 blink (200 ms on, 200 ms off).
- FAIL: 3 rapid blinks (100 ms on, 100 ms off × 3).
- `gpio_config_t` with `GPIO_MODE_OUTPUT`.

## Logging

- Tag: `"aes-demo"`
- Init: `ESP_LOGI(TAG, "AES key %s NVS", key_was_loaded ? "loaded from" : "generated, saved to")`
- Self-test: `ESP_LOGI(TAG, "Self-test: %s", passed ? "PASS" : "FAIL")`
- Benchmark: log throughput in MB/s.

## sdkconfig.defaults

```
CONFIG_MBEDTLS_HARDWARE_AES=y
CONFIG_ESP_CONSOLE_USB_CDC=y
```

## Error Handling

- `ESP_ERROR_CHECK` on all NVS and GPIO calls.
- mbedTLS functions return int; check for non-zero return and log error with `mbedtls_strerror`.
- On self-test FAIL: log error but do not abort — continue to next iteration (example should not crash on demo boards).

## Agent-Generated Headers

- `.c`/`.h` files: `/* AGENT-GENERATED — do not edit by hand; regenerate via esp32-sdd-full-project-generator */`
- CMakeLists.txt, sdkconfig.defaults, .gitignore: `# AGENT-GENERATED — do not edit by hand; regenerate via esp32-sdd-full-project-generator`
- README.md: full HTML comment block.

## File Layout

- main/main.c
- main/secure_storage.c
- main/secure_storage.h
- main/CMakeLists.txt
- CMakeLists.txt
- sdkconfig.defaults
- .gitignore
- README.md
