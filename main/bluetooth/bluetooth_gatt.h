#ifndef BLUETOOTH_COMM_GATT_H
#define BLUETOOTH_COMM_GATT_H

#include <stdint.h>
#include <stddef.h>
#include "nimble/ble.h"
#include "host/ble_hs.h"
#include "host/ble_gap.h"
#include "host/ble_gatt.h"
#include "esp_log.h"

#ifdef __cplusplus
extern "C" {
#endif

// Global variable to track the current connection handle
extern uint16_t current_conn_handle;

// Global variable for the characteristic handle
extern uint16_t custom_chr_handle;

// Global flag to enable/disable notifications at runtime
extern bool enable_notifications;

// Buffer to store data received from the client
extern uint8_t received_data[128];
extern size_t received_data_len;

// Enable or disable notifications at runtime


// Function to initialize the custom GATT service
void gatt_init(void);

// Function to send a notification on the custom characteristic
int custom_send_notification(const uint8_t *data, uint16_t len);

// GAP event callback to handle connection and disconnection


// BLE host task to manage NimBLE host stack


// Callback executed when the NimBLE host is synchronized
void ble_app_on_sync(void);

// Send data via GATT characteristic
void bluetooth_comm_gatt_send_data(const uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif // BLUETOOTH_COMM_GATT_H
