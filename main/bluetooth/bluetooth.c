#include "bluetooth.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h" // Include the header for BLE host stack
#include "services/gap/ble_svc_gap.h"
#include "esp_log.h"

 //static const char *TAG = "Bluetooth";
 static const char *TAG = "Bluetooth";

// Callback for BLE stack reset
void ble_on_reset(int reason) {
    ESP_LOGI(TAG, "BLE stack reset; reason=%d", reason);
}

// Callback for BLE stack sync
void ble_app_on_sync(void) {
    ESP_LOGI(TAG, "BLE stack synced, starting advertising...");

    // Set advertising parameters
    struct ble_gap_adv_params adv_params = {0};
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

    // Start advertising
    int rc = ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, NULL, BLE_HS_FOREVER, &adv_params, NULL, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to start advertising; rc=%d", rc); // Log the error code directly
    } else {
        ESP_LOGI(TAG, "Advertising started successfully");
    }
}

// NimBLE host task

// BLE initialization
void ble_init(void) {
    // Initialize NimBLE
    nimble_port_init();

    // Configure the NimBLE host stack
    ble_hs_cfg.reset_cb = ble_on_reset;
    ble_hs_cfg.sync_cb = ble_app_on_sync; // Use ble_app_on_sync as the sync callback
    ble_hs_cfg.gatts_register_cb = gatt_svr_register_cb;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

    // Set the device name
    ble_svc_gap_device_name_set("NimBLE-Device");

    // Start the NimBLE host task
   // nimble_port_freertos_init(ble_host_task);
}



void bluetooth_send_data(const uint8_t *data, size_t len) {
    // Send data over Bluetooth here
    ESP_LOGI("TAG", "Sending data over Bluetooth");
}