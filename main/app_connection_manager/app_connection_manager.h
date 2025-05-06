#ifndef APP_CONNECTION_MANAGER_H
#define APP_CONNECTION_MANAGER_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_wifi_types.h"  // Added for wifi_auth_mode_t

typedef struct {
    bool wifi_connected;
    bool bluetooth_connected;
    char connected_ssid[32];  // Store connected SSID
} connection_status_t;

// Wi-Fi scan result structure
typedef struct {
    char ssid[32];          // Network name
    int8_t rssi;            // Signal strength in dBm
    wifi_auth_mode_t auth_mode;  // Authentication mode (using standard esp_wifi type)
} wifi_ap_info_t;

/**
 * @brief Initialize connection manager
 */
void app_connection_manager_init(void);

/**
 * @brief Update WiFi connection status
 * @param connected True if connected
 * @param ssid Connected network SSID (nullable)
 */
void app_connection_manager_set_wifi_status(bool connected, const char *ssid);

/**
 * @brief Update Bluetooth connection status
 * @param connected True if connected
 */
void app_connection_manager_set_bluetooth_status(bool connected);

/**
 * @brief Send data through active connections
 * @param data Null-terminated string to send
 */
void app_connection_manager_send_data(const char *data);

/* ----------------- Wi-Fi Scanning Functions ---------------- */

/**
 * @brief Start asynchronous WiFi scan
 */
void app_connection_manager_start_wifi_scan(void);

/**
 * @brief Get scan results with automatic printing
 * @param results Buffer to store results
 * @param max_results Capacity of results buffer
 * @return Number of networks found (or negative for error)
 * 
 * @note This function will automatically print found networks to log
 */
int app_connection_manager_get_scan_results(wifi_ap_info_t *results, int max_results);

/**
 * @brief Connect to specified AP
 * @param ssid Network name
 * @param password Network password
 * @return True if connection attempt started successfully
 */
bool app_connection_manager_connect_to_ap(const char *ssid, const char *password);

/* ----------------- Helper Functions ---------------- */

/**
 * @brief Convert auth mode to readable string
 * @param auth_mode Authentication mode from scan results
 * @return Constant string describing auth mode
 */
const char* auth_mode_to_string(wifi_auth_mode_t auth_mode);

#endif // APP_CONNECTION_MANAGER_H