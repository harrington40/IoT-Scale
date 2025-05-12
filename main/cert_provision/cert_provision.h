#ifndef CERT_PROVISION_H
#define CERT_PROVISION_H

#include "nvs_flash.h"
#include "nvs.h"
#include <stdint.h>
#include <esp_err.h>

// Global certificate pointers
extern const uint8_t *server_cert_chain_pem;
extern size_t server_cert_chain_len;
extern const uint8_t *server_private_key_pem;
extern size_t server_private_key_len;

/**
 * @brief Provision certificates to NVS
 * @param cert_data Certificate chain in PEM format
 * @param cert_len Length of certificate data
 * @param key_data Private key in PEM format
 * @param key_len Length of private key data
 * @return ESP_OK on success, error code on failure
 */
esp_err_t provision_certs_to_nvs(const uint8_t *cert_data, size_t cert_len,
                               const uint8_t *key_data, size_t key_len);

/**
 * @brief Load certificates from NVS into memory
 * @return ESP_OK on success, error code on failure
 */
esp_err_t load_certs_from_nvs(void);

/**
 * @brief Free allocated certificate buffers
 */
void free_cert_buffers(void);

#endif // CERT_PROVISION_H