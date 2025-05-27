#include "wifi_power_management.h"
#include "esp_wifi.h"
#include "esp_bt.h"
#include "esp_log.h"
#include "esp_sleep.h"

// Remove Bluedroid header:
// #include "esp_gap_ble_api.h"

// Include NimBLE headers for BLE APIs.



// Tag for logging
static const char *TAG = "Wi-Fi Power Management";

// Global BLE connection handle.
// This should be maintained by your BLE module.


// Define BLE connection update parameters for low-power operation.

//------------------------------------------------------------------------------
// Power Management Functions Using NimBLE
//------------------------------------------------------------------------------

// Manage power modes for Wi-Fi and BLE based on activity.
void manage_power_modes(bool is_wifi_active, bool is_ble_active)
{
    if (is_wifi_active && is_ble_active) {
        wifi_active_mode();
        ble_idle_mode(); // Stop BLE advertising if idle.
    } else if (is_wifi_active && !is_ble_active) {
        wifi_active_mode();
        ble_advertising_sleep();
    } else if (!is_wifi_active && is_ble_active) {
        wifi_modem_sleep();
        ble_connection_sleep();
    } else {
        light_sleep();
    }
}

// Put Wi-Fi in modem sleep (low-power mode for Wi-Fi).
void wifi_modem_sleep(void)
{
    esp_err_t err = esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Wi-Fi modem sleep (MIN_MODEM) enabled");
    } else {
        ESP_LOGE(TAG, "Failed to set Wi-Fi modem sleep: %s", esp_err_to_name(err));
    }
}

// Approximate Wi-Fi light sleep using MAX_MODEM.
void wifi_light_sleep(void)
{
    esp_err_t err = esp_wifi_set_ps(WIFI_PS_MAX_MODEM);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Wi-Fi 'light sleep' (MAX_MODEM) enabled (approximation)");
    } else {
        ESP_LOGE(TAG, "Failed to set Wi-Fi power save mode: %s", esp_err_to_name(err));
    }
}

// Switch Wi-Fi to active mode (disable power saving).
void wifi_active_mode(void)
{
    esp_err_t err = esp_wifi_set_ps(WIFI_PS_NONE);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Wi-Fi active mode enabled (no power save)");
    } else {
        ESP_LOGE(TAG, "Failed to set Wi-Fi active mode: %s", esp_err_to_name(err));
    }
}

// Put BLE into advertising sleep mode (stop advertising) using NimBLE.


// Update BLE connection parameters for power saving using NimBLE.


// Put BLE into idle mode (stop advertising) using NimBLE.
void ble_idle_mode(void)
{
    int rc = ble_gap_adv_stop();
    if (rc == 0) {
        ESP_LOGI(TAG, "BLE idle mode enabled (advertising stopped)");
    } else {
        ESP_LOGE(TAG, "Failed to stop BLE advertising; rc=%d", rc);
    }
}

// Enter deep sleep (turn off Wi-Fi, BLE, and CPU).
void enter_deep_sleep(void)
{
    ESP_LOGI(TAG, "Entering deep sleep...");
    // Configure wakeup sources as needed.
    esp_deep_sleep_start();
}

// Put both Wi-Fi and BLE into light sleep during inactivity.
void light_sleep(void)
{
    wifi_modem_sleep();      // Put Wi-Fi into modem-sleep.
    ble_advertising_sleep(); // Stop BLE advertising.
    ESP_LOGI(TAG, "Entering 'light sleep' mode for inactivity");
}

// Update BLE advertising interval using NimBLE.
// Note: In some ESP-IDF versions with NimBLE, advertising interval parameters
// are set during advertising start. Here we restart advertising with new parameters.
void set_ble_advertising_interval(uint16_t min_interval, uint16_t max_interval)
{
    struct ble_gap_adv_params adv_params = {
        .conn_mode = BLE_GAP_CONN_MODE_UND,
        .disc_mode = BLE_GAP_DISC_MODE_GEN,
        .itvl_min = min_interval,  // in 0.625 ms units
        .itvl_max = max_interval,  // in 0.625 ms units
    };

    int rc = ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, NULL, BLE_HS_FOREVER,
                               &adv_params, NULL, NULL);
    if (rc == 0) {
        ESP_LOGI(TAG, "BLE advertising interval set: %d - %d (units of 0.625 ms)",
                 min_interval, max_interval);
    } else {
        ESP_LOGE(TAG, "Failed to start BLE advertising with new interval; rc=%d", rc);
    }
}
