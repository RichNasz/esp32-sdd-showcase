// ================================================
// AGENT-GENERATED — DO NOT EDIT BY HAND
// Generated from specs/ using esp32-sdd-full-project-generator skill
// Date: 2026-03-09 | Agent: Claude Code
// ================================================

#include "secure_storage.h"

#include <string.h>
#include <stdlib.h>

#include "esp_log.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "mbedtls/aes.h"
#include "mbedtls/error.h"

static const char *TAG = "secure_storage";

#define AES_KEY_BITS    256
#define AES_KEY_BYTES   (AES_KEY_BITS / 8)   /* 32 */
#define AES_BLOCK_SIZE  16

/* NVS namespaces — max 15 chars each */
#define NVS_NS_KEYS     "sec_store"
#define NVS_NS_BLOBS    "sec_blobs"
#define NVS_KEY_AES     "aes_key"

static uint8_t s_aes_key[AES_KEY_BYTES];
static bool    s_initialized = false;

/* --------------------------------------------------------------------------
 * PKCS7 helpers
 * -------------------------------------------------------------------------- */

/* Number of padding bytes to append so that (len + result) % AES_BLOCK_SIZE == 0.
 * Always returns 1..AES_BLOCK_SIZE (a full block is added when already aligned,
 * which makes unpadding unambiguous). */
static size_t pkcs7_pad_count(size_t len)
{
    return (size_t)(AES_BLOCK_SIZE - (len % AES_BLOCK_SIZE));
}

/* Write PKCS7-padded plaintext into @p out (must be padded_len bytes). */
static void pkcs7_apply(const uint8_t *in, size_t in_len,
                        uint8_t *out, size_t padded_len)
{
    uint8_t pad_byte = (uint8_t)(padded_len - in_len);
    memcpy(out, in, in_len);
    memset(out + in_len, pad_byte, pad_byte);
}

/* Validate PKCS7 padding and return the plaintext length, or 0 on bad padding. */
static size_t pkcs7_strip(const uint8_t *buf, size_t buf_len)
{
    if (buf_len == 0 || buf_len % AES_BLOCK_SIZE != 0) {
        return 0;
    }
    uint8_t pad_byte = buf[buf_len - 1];
    if (pad_byte == 0 || pad_byte > AES_BLOCK_SIZE) {
        return 0;
    }
    for (size_t i = buf_len - pad_byte; i < buf_len; i++) {
        if (buf[i] != pad_byte) {
            return 0;
        }
    }
    return buf_len - pad_byte;
}

/* --------------------------------------------------------------------------
 * AES-CBC helpers — hardware acceleration is transparent via mbedTLS when
 * CONFIG_MBEDTLS_HARDWARE_AES=y; the call sites do not change.
 * -------------------------------------------------------------------------- */

static esp_err_t aes_cbc_encrypt(const uint8_t *iv, const uint8_t *pt,
                                  size_t pt_len, uint8_t *ct)
{
    /* mbedtls_aes_crypt_cbc() updates iv in-place for CBC chaining; copy first. */
    uint8_t iv_buf[AES_BLOCK_SIZE];
    memcpy(iv_buf, iv, AES_BLOCK_SIZE);

    mbedtls_aes_context ctx;
    mbedtls_aes_init(&ctx);
    int ret = mbedtls_aes_setkey_enc(&ctx, s_aes_key, AES_KEY_BITS);
    if (ret == 0) {
        ret = mbedtls_aes_crypt_cbc(&ctx, MBEDTLS_AES_ENCRYPT,
                                     pt_len, iv_buf, pt, ct);
    }
    mbedtls_aes_free(&ctx);

    if (ret != 0) {
        char errbuf[80];
        mbedtls_strerror(ret, errbuf, sizeof(errbuf));
        ESP_LOGE(TAG, "AES encrypt error: %s", errbuf);
        return ESP_FAIL;
    }
    return ESP_OK;
}

static esp_err_t aes_cbc_decrypt(const uint8_t *iv, const uint8_t *ct,
                                  size_t ct_len, uint8_t *pt)
{
    uint8_t iv_buf[AES_BLOCK_SIZE];
    memcpy(iv_buf, iv, AES_BLOCK_SIZE);

    mbedtls_aes_context ctx;
    mbedtls_aes_init(&ctx);
    int ret = mbedtls_aes_setkey_dec(&ctx, s_aes_key, AES_KEY_BITS);
    if (ret == 0) {
        ret = mbedtls_aes_crypt_cbc(&ctx, MBEDTLS_AES_DECRYPT,
                                     ct_len, iv_buf, ct, pt);
    }
    mbedtls_aes_free(&ctx);

    if (ret != 0) {
        char errbuf[80];
        mbedtls_strerror(ret, errbuf, sizeof(errbuf));
        ESP_LOGE(TAG, "AES decrypt error: %s", errbuf);
        return ESP_FAIL;
    }
    return ESP_OK;
}

/* --------------------------------------------------------------------------
 * Key management
 * -------------------------------------------------------------------------- */

static esp_err_t load_or_generate_key(void)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NS_KEYS, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nvs_open(%s) failed: %s", NVS_NS_KEYS, esp_err_to_name(err));
        return err;
    }

    size_t key_len = AES_KEY_BYTES;
    err = nvs_get_blob(handle, NVS_KEY_AES, s_aes_key, &key_len);

    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(TAG, "AES key not found; generating and saving to NVS");
        esp_fill_random(s_aes_key, AES_KEY_BYTES);
        err = nvs_set_blob(handle, NVS_KEY_AES, s_aes_key, AES_KEY_BYTES);
        if (err == ESP_OK) {
            err = nvs_commit(handle);
        }
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "AES key generated and saved to NVS");
        } else {
            ESP_LOGE(TAG, "Failed to save AES key: %s", esp_err_to_name(err));
        }
    } else if (err == ESP_OK) {
        ESP_LOGI(TAG, "AES key loaded from NVS");
    } else {
        ESP_LOGE(TAG, "nvs_get_blob(%s) failed: %s", NVS_KEY_AES, esp_err_to_name(err));
    }

    nvs_close(handle);
    return err;
}

/* --------------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------------- */

esp_err_t secure_storage_init(void)
{
    if (s_initialized) {
        return ESP_OK;
    }
    esp_err_t err = load_or_generate_key();
    if (err == ESP_OK) {
        s_initialized = true;
    }
    return err;
}

esp_err_t secure_write(const char *key, const void *data, size_t len)
{
    if (!s_initialized || !key || !data || len == 0) {
        return ESP_ERR_INVALID_STATE;
    }

    size_t pad_bytes = pkcs7_pad_count(len);
    size_t pad_len   = len + pad_bytes;
    /* Single allocation: [IV (16)] [ciphertext (pad_len)] [padded plaintext scratch] */
    size_t blob_len  = AES_BLOCK_SIZE + pad_len;
    uint8_t *buf     = malloc(blob_len + pad_len);
    if (!buf) {
        ESP_LOGE(TAG, "malloc(%zu) failed", blob_len + pad_len);
        return ESP_ERR_NO_MEM;
    }

    uint8_t *iv_area  = buf;
    uint8_t *ct_area  = buf + AES_BLOCK_SIZE;
    uint8_t *padded   = buf + blob_len;          /* scratch: PKCS7-padded plaintext */

    esp_fill_random(iv_area, AES_BLOCK_SIZE);
    pkcs7_apply((const uint8_t *)data, len, padded, pad_len);

    esp_err_t err = aes_cbc_encrypt(iv_area, padded, pad_len, ct_area);
    if (err != ESP_OK) {
        free(buf);
        return err;
    }

    nvs_handle_t handle;
    err = nvs_open(NVS_NS_BLOBS, NVS_READWRITE, &handle);
    if (err == ESP_OK) {
        err = nvs_set_blob(handle, key, buf, blob_len);
        if (err == ESP_OK) {
            err = nvs_commit(handle);
        }
        nvs_close(handle);
    }

    free(buf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "secure_write(%s) failed: %s", key, esp_err_to_name(err));
    }
    return err;
}

esp_err_t secure_read(const char *key, void *data, size_t *len)
{
    if (!s_initialized || !key || !data || !len) {
        return ESP_ERR_INVALID_STATE;
    }

    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NS_BLOBS, NVS_READONLY, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nvs_open(%s) failed: %s", NVS_NS_BLOBS, esp_err_to_name(err));
        return err;
    }

    /* Query blob size before allocating. */
    size_t blob_len = 0;
    err = nvs_get_blob(handle, key, NULL, &blob_len);
    if (err != ESP_OK) {
        nvs_close(handle);
        return err;   /* ESP_ERR_NVS_NOT_FOUND propagates cleanly */
    }

    if (blob_len <= AES_BLOCK_SIZE) {
        nvs_close(handle);
        ESP_LOGE(TAG, "Blob for '%s' too short (%zu bytes)", key, blob_len);
        return ESP_ERR_INVALID_SIZE;
    }

    uint8_t *blob = malloc(blob_len);
    if (!blob) {
        nvs_close(handle);
        return ESP_ERR_NO_MEM;
    }

    err = nvs_get_blob(handle, key, blob, &blob_len);
    nvs_close(handle);
    if (err != ESP_OK) {
        free(blob);
        return err;
    }

    size_t ct_len = blob_len - AES_BLOCK_SIZE;
    uint8_t *iv   = blob;
    uint8_t *ct   = blob + AES_BLOCK_SIZE;

    uint8_t *pt = malloc(ct_len);
    if (!pt) {
        free(blob);
        return ESP_ERR_NO_MEM;
    }

    err = aes_cbc_decrypt(iv, ct, ct_len, pt);
    free(blob);

    if (err != ESP_OK) {
        free(pt);
        return err;
    }

    size_t pt_len = pkcs7_strip(pt, ct_len);
    if (pt_len == 0) {
        ESP_LOGE(TAG, "PKCS7 unpad failed for '%s' — corrupt blob or wrong key", key);
        free(pt);
        return ESP_ERR_INVALID_CRC;
    }

    if (pt_len > *len) {
        ESP_LOGE(TAG, "Buffer too small for '%s': need %zu, have %zu", key, pt_len, *len);
        free(pt);
        return ESP_ERR_INVALID_SIZE;
    }

    memcpy(data, pt, pt_len);
    *len = pt_len;
    free(pt);
    return ESP_OK;
}

esp_err_t secure_storage_benchmark(size_t data_len, float *throughput_mbs)
{
    if (!s_initialized || !throughput_mbs) {
        return ESP_ERR_INVALID_STATE;
    }
    if (data_len == 0 || data_len % AES_BLOCK_SIZE != 0) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t *plaintext  = malloc(data_len);
    uint8_t *ciphertext = malloc(data_len);
    if (!plaintext || !ciphertext) {
        free(plaintext);
        free(ciphertext);
        return ESP_ERR_NO_MEM;
    }

    esp_fill_random(plaintext, data_len);

    uint8_t iv[AES_BLOCK_SIZE];
    esp_fill_random(iv, AES_BLOCK_SIZE);

    /* Time the hardware-accelerated encryption only (no NVS I/O). */
    int64_t t0 = esp_timer_get_time();
    esp_err_t err = aes_cbc_encrypt(iv, plaintext, data_len, ciphertext);
    int64_t t1 = esp_timer_get_time();

    free(plaintext);
    free(ciphertext);

    if (err == ESP_OK) {
        int64_t elapsed_us = t1 - t0;
        /* bytes / µs = MB/s (since 1 MB = 10^6 bytes and 1 s = 10^6 µs) */
        *throughput_mbs = (elapsed_us > 0)
                          ? (float)data_len / (float)elapsed_us
                          : 0.0f;
    }
    return err;
}
