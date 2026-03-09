// ================================================
// AGENT-GENERATED — DO NOT EDIT BY HAND
// Generated from specs/ using esp32-sdd-full-project-generator skill
// Date: 2026-03-09 | Agent: Claude Code
// ================================================
#pragma once

#include "esp_err.h"
#include <stddef.h>

/**
 * @brief Initialise the secure storage module.
 *
 * Loads the AES-256 key from NVS, generating and persisting a fresh key
 * on first boot using the hardware CSPRNG. Must be called once, after
 * nvs_flash_init(), before any other function in this module.
 *
 * @return ESP_OK on success, or an esp_err_t propagated from NVS.
 */
esp_err_t secure_storage_init(void);

/**
 * @brief Encrypt @p data and store it in NVS under @p key.
 *
 * A fresh random IV is generated per call (IV reuse with the same key
 * would break CBC confidentiality). The stored blob layout is:
 *   [16-byte IV] [PKCS7-padded ciphertext]
 *
 * @p key follows NVS key rules: max 15 characters, null-terminated.
 *
 * @param key   NVS key name (max 15 chars).
 * @param data  Plaintext buffer to encrypt.
 * @param len   Length of @p data in bytes (must be > 0).
 * @return ESP_OK on success, ESP_ERR_NO_MEM on allocation failure,
 *         or an esp_err_t from mbedTLS / NVS.
 */
esp_err_t secure_write(const char *key, const void *data, size_t len);

/**
 * @brief Retrieve and decrypt a blob previously stored by secure_write().
 *
 * On entry, *len must be the capacity of @p data in bytes.
 * On success, *len is updated to the actual plaintext length.
 *
 * @param key   NVS key name (max 15 chars).
 * @param data  Buffer to receive the decrypted plaintext.
 * @param len   [in] capacity of @p data; [out] actual plaintext length.
 * @return ESP_OK on success, ESP_ERR_NVS_NOT_FOUND if key is absent,
 *         ESP_ERR_INVALID_SIZE if the buffer is too small,
 *         ESP_ERR_INVALID_CRC if PKCS7 validation fails (wrong key or
 *         corrupt blob), or an esp_err_t from mbedTLS / NVS.
 */
esp_err_t secure_read(const char *key, void *data, size_t *len);

/**
 * @brief Benchmark AES-256-CBC hardware throughput.
 *
 * Encrypts @p data_len bytes of random data in memory (no NVS I/O) and
 * returns the measured throughput. Use a multiple of 16 for @p data_len.
 *
 * @param data_len        Bytes to encrypt; must be a non-zero multiple of 16.
 * @param throughput_mbs  [out] Measured throughput in MB/s.
 * @return ESP_OK on success, ESP_ERR_NO_MEM on allocation failure,
 *         ESP_ERR_INVALID_ARG if data_len is 0 or not block-aligned.
 */
esp_err_t secure_storage_benchmark(size_t data_len, float *throughput_mbs);
