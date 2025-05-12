#include "cert_provision.h"
#include "esp_log.h"
#include <stdlib.h>
#include <string.h>
#include <mem.h>   // for memmem

static const char *TAG = "cert_prov";

// Marker to look for between certs in a CA-signed chain
static const char *chain_marker = "-----END CERTIFICATE-----\n-----BEGIN CERTIFICATE-----";

// Global certificate pointers
const uint8_t *server_cert_chain_pem = NULL;
size_t server_cert_chain_len = 0;
const uint8_t *server_private_key_pem = NULL;
size_t server_private_key_len = 0;

// … validate_pem() and provision_certs_to_nvs() remain unchanged …

esp_err_t load_certs_from_nvs(void) {
    esp_err_t err;
    nvs_handle_t h;
    size_t full_len = 0, key_len = 0;

    // Initialize NVS
    if ((err = nvs_flash_init()) != ESP_OK) {
        ESP_LOGE(TAG, "NVS init failed: %s", esp_err_to_name(err));
        return err;
    }

    // Open NVS namespace
    if ((err = nvs_open("tls", NVS_READONLY, &h)) != ESP_OK) {
        ESP_LOGE(TAG, "nvs_open failed: %s", esp_err_to_name(err));
        return err;
    }

    // Get blob sizes
    if ((err = nvs_get_blob(h, "fullchain", NULL, &full_len)) != ESP_OK ||
        (err = nvs_get_blob(h, "prvtkey",   NULL, &key_len))   != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get blob sizes: %s", esp_err_to_name(err));
        nvs_close(h);
        return err;
    }

    // Allocate buffers
    uint8_t *cert_buf = malloc(full_len);
    uint8_t *key_buf  = malloc(key_len);
    if (!cert_buf || !key_buf) {
        free(cert_buf);
        free(key_buf);
        nvs_close(h);
        ESP_LOGE(TAG, "Memory allocation failed");
        return ESP_ERR_NO_MEM;
    }

    // Read blobs
    if ((err = nvs_get_blob(h, "fullchain", cert_buf, &full_len)) != ESP_OK ||
        (err = nvs_get_blob(h, "prvtkey",   key_buf,  &key_len))   != ESP_OK) {
        free(cert_buf);
        free(key_buf);
        nvs_close(h);
        ESP_LOGE(TAG, "Failed to read blobs: %s", esp_err_to_name(err));
        return err;
    }

    // Update globals
    server_cert_chain_pem    = cert_buf;
    server_cert_chain_len    = full_len;
    server_private_key_pem   = key_buf;
    server_private_key_len   = key_len;

    // --- New debugging: find and print chain marker + entire PEM ---
    void *marker_ptr = memmem(cert_buf, full_len,
                              chain_marker, strlen(chain_marker));
    if (marker_ptr) {
        size_t offset = (uint8_t*)marker_ptr - cert_buf;
        ESP_LOGI(TAG, "Chain marker found at offset %u", (unsigned)offset);
    } else {
        ESP_LOGW(TAG, "Chain marker NOT found in loaded PEM!");
    }
    // Dump the whole chain (incl. marker) as a human-readable log
    ESP_LOGI(TAG, "Full certificate chain PEM:\n%.*s",
             (int)full_len, cert_buf);
    // -------------------------------------------------------------

    nvs_close(h);
    return ESP_OK;
}

void free_cert_buffers(void) {
    free((void*)server_cert_chain_pem);
    free((void*)server_private_key_pem);
    server_cert_chain_pem    = NULL;
    server_private_key_pem   = NULL;
    server_cert_chain_len    = 0;
    server_private_key_len   = 0;
}
