#include "app_connection_manager.h"
#include "bluetooth_comm_gatt.h"
#include "wifi_comm.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "freertos/event_groups.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "APP_CONN_MGR";

// Connection status structure
static connection_status_t connection_status = {
    .wifi_connected = false,
    .bluetooth_connected = false,
    .connected_ssid = ""
};

// Wi-Fi scanning variables
static EventGroupHandle_t wifi_scan_event_group;
const int WIFI_SCAN_DONE_BIT = BIT0;

/* ------------------------- Private Functions ------------------------- */

static void wifi_scan_event_handler(void* arg, esp_event_base_t event_base, 
                                  int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_SCAN_DONE) {
        xEventGroupSetBits(wifi_scan_event_group, WIFI_SCAN_DONE_BIT);
    }
}

 const char* auth_mode_to_string(wifi_auth_mode_t auth_mode) {
    switch(auth_mode) {
        case WIFI_AUTH_OPEN: return "Open";
        case WIFI_AUTH_WEP: return "WEP";
        case WIFI_AUTH_WPA_PSK: return "WPA";
        case WIFI_AUTH_WPA2_PSK: return "WPA2";
        case WIFI_AUTH_WPA_WPA2_PSK: return "WPA/WPA2";
        case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2-Enterprise";
        case WIFI_AUTH_WPA3_PSK: return "WPA3";
        case WIFI_AUTH_WPA2_WPA3_PSK: return "WPA2/WPA3";
        default: return "Unknown";
    }
}

/* ------------------------- Public Functions ------------------------- */

void app_connection_manager_init(void) {
    // Initialize Wi-Fi
    wifi_init();
    
    // Initialize Bluetooth
    custom_gatt_init();

    // Create event group for Wi-Fi scanning
    wifi_scan_event_group = xEventGroupCreate();
    
    // Register Wi-Fi scan event handler
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, 
        WIFI_EVENT_SCAN_DONE,
        &wifi_scan_event_handler,
        NULL,
        NULL
    ));

    ESP_LOGI(TAG, "Connection manager initialized");
}

void app_connection_manager_set_wifi_status(bool connected, const char *ssid) {
    connection_status.wifi_connected = connected;
    if (connected && ssid) {
        strncpy(connection_status.connected_ssid, ssid, 
               sizeof(connection_status.connected_ssid)-1);
        connection_status.connected_ssid[sizeof(connection_status.connected_ssid)-1] = '\0';
    } else {
        connection_status.connected_ssid[0] = '\0';
    }
    ESP_LOGI(TAG, "Wi-Fi %s to %s", 
             connected ? "connected" : "disconnected", 
             ssid ? ssid : "N/A");
}

void app_connection_manager_set_bluetooth_status(bool connected) {
    connection_status.bluetooth_connected = connected;
    ESP_LOGI(TAG, "Bluetooth %s", connected ? "connected" : "disconnected");
}

void app_connection_manager_send_data(const char *data) {
    if (!data) {
        ESP_LOGE(TAG, "Null data pointer");
        return;
    }

    if (connection_status.wifi_connected) {
        wifi_send_data(data);
        ESP_LOGD(TAG, "Sent over Wi-Fi: %s", data);
    }

    if (connection_status.bluetooth_connected) {
        bluetooth_comm_gatt_send_data((const uint8_t *)data, strlen(data));
        ESP_LOGD(TAG, "Sent over Bluetooth: %s", data);
    }

    if (!connection_status.wifi_connected && !connection_status.bluetooth_connected) {
        ESP_LOGW(TAG, "No active connections to send data");
    }
}

/* ------------------------- Wi-Fi Functions ------------------------- */

void app_connection_manager_start_wifi_scan(void) {
    // Configure and start scan
    wifi_scan_config_t scan_config = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .show_hidden = false
    };

    // Clear any previous scan done flag
    xEventGroupClearBits(wifi_scan_event_group, WIFI_SCAN_DONE_BIT);
    
    ESP_ERROR_CHECK(esp_wifi_scan_start(&scan_config, false));
    ESP_LOGI(TAG, "Wi-Fi scan started");
}

int app_connection_manager_get_scan_results(wifi_ap_info_t *results, int max_results) {
    if (!results || max_results <= 0) {
        ESP_LOGE(TAG, "Invalid parameters for scan results");
        return 0;
    }

    // Wait for scan completion (10s timeout)
    EventBits_t bits = xEventGroupWaitBits(
        wifi_scan_event_group,
        WIFI_SCAN_DONE_BIT,
        pdFALSE,
        pdFALSE,
        pdMS_TO_TICKS(10000)
    );

    if ((bits & WIFI_SCAN_DONE_BIT) == 0) {
        ESP_LOGE(TAG, "Wi-Fi scan timed out");
        return 0;
    }

    // Get number of APs found
    uint16_t ap_count = 0;
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
    if (ap_count == 0) {
        ESP_LOGI(TAG, "No Wi-Fi networks found");
        return 0;
    }

    // Allocate temporary storage
    wifi_ap_record_t *ap_records = malloc(sizeof(wifi_ap_record_t) * ap_count);
    if (!ap_records) {
        ESP_LOGE(TAG, "Failed to allocate memory for scan results");
        return 0;
    }

    // Get records and copy to output
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_count, ap_records));
    
    // Print header
    ESP_LOGI(TAG, "----------------------------------");
    ESP_LOGI(TAG, "Found %d Wi-Fi networks:", ap_count);
    ESP_LOGI(TAG, "----------------------------------");
    
    int result_count = (ap_count < max_results) ? ap_count : max_results;
    for (int i = 0; i < result_count; i++) {
        // Copy to results
        strncpy(results[i].ssid, (char*)ap_records[i].ssid, sizeof(results[i].ssid)-1);
        results[i].ssid[sizeof(results[i].ssid)-1] = '\0';
        results[i].rssi = ap_records[i].rssi;
        results[i].auth_mode = ap_records[i].authmode;
        
        // Print each network
        ESP_LOGI(TAG, "%2d. %-32s RSSI: %3ddBm %s", 
                i+1,
                results[i].ssid,
                results[i].rssi,
                auth_mode_to_string(results[i].auth_mode));
    }
    ESP_LOGI(TAG, "----------------------------------");

    free(ap_records);
    return result_count;
}

bool app_connection_manager_connect_to_ap(const char *ssid, const char *password) {
    if (!ssid || !password) {
        ESP_LOGE(TAG, "Invalid SSID or password");
        return false;
    }

    // Disconnect first if already connected
    esp_wifi_disconnect();

    // Configure WiFi station
    wifi_config_t wifi_config = {0};
    strncpy((char*)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char*)wifi_config.sta.password, password, sizeof(wifi_config.sta.password) - 1);
    
    // Important configuration options
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    wifi_config.sta.pmf_cfg.capable = true;
    wifi_config.sta.pmf_cfg.required = false;
    
    // Set configuration
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    
    // Attempt connection with timeout
    esp_err_t err = esp_wifi_connect();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "WiFi connection failed to start: %s", esp_err_to_name(err));
        return false;
    }

    ESP_LOGI(TAG, "WiFi connection attempt started to SSID: %s", ssid);
    return true;
}