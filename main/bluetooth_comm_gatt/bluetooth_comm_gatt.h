#ifndef BLUETOOTH_COMM_GATT_H
#define BLUETOOTH_COMM_GATT_H

#include <stdint.h>
#include "nimble/ble.h"
#include "host/ble_hs.h"
#include "host/ble_gap.h"
#include "host/ble_gatt.h"
#include "esp_log.h"

#ifdef __cplusplus
extern "C" {
#endif

// Logging tag
static const char *TAG;

// Global variable to track the current connection handle
extern uint16_t current_conn_handle;

// Global variable for the characteristic handle
extern uint16_t custom_chr_handle;

// Buffer to store data received from the client
extern uint8_t received_data[128];
extern size_t received_data_len;

// Enable or disable notifications (1 = enabled, 0 = disabled)
#define ENABLE_NOTIFICATIONS 1


// Function to initialize the custom GATT service
void custom_gatt_init(void);

// Function to send a notification on the custom characteristic
int custom_send_notification(const uint8_t *data, uint16_t len);

// Function to enable/disable notifications at runtime
void set_notifications_enabled(bool enabled);

// GAP event callback to handle connection and disconnection
int ble_gap_event(struct ble_gap_event *event, void *arg);

// Callback executed when the NimBLE host is synchronized
extern void ble_app_on_sync(void);

// BLE host task
void ble_host_task(void *param);
void bluetooth_comm_gatt_send_data(const uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif // BLUETOOTH_COMM_GATT_H