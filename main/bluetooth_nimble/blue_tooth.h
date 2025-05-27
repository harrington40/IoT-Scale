#ifndef BLUETOOTH_NIMBLE_H
#define BLUETOOTH_NIMBLE_H

#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the BLE stack and services
 * @return esp_err_t ESP_OK on success, error code otherwise
 */
esp_err_t bluetooth_nimble_init(void);

/**
 * @brief Deinitialize the BLE stack
 * @return esp_err_t ESP_OK on success, error code otherwise
 */
esp_err_t bluetooth_nimble_deinit(void);

/**
 * @brief Update weight measurement
 * @param weight_kg Weight in kilograms
 * @param stable Whether the measurement is stable
 */
void bluetooth_update_weight(float weight_kg, bool stable);

/**
 * @brief Update body fat percentage
 * @param percentage Body fat percentage (0-100)
 */
void bluetooth_update_fat_percentage(float percentage);

/**
 * @brief Check if BLE is connected
 * @return true if connected, false otherwise
 */
bool bluetooth_is_connected(void);

/**
 * @brief Restart BLE advertising
 */
void bluetooth_restart_advertising(void);

/**
 * @brief Disconnect current BLE connection
 */
void bluetooth_disconnect_current(void);

void start_advertising(void);
/**
 * @brief Check if advertising is active
 * @return true if advertising, false otherwise
 */
int ble_gap_adv_active(void);

#ifdef __cplusplus
}
#endif

#endif // BLUETOOTH_NIMBLE_H