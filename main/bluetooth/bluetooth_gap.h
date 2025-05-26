#ifndef BLUETOOTH_GAP_H
#define BLUETOOTH_GAP_H

#include <stdint.h>
#include <stdbool.h>
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "services/gap/ble_svc_gap.h"
#include "esp_log.h"
#include "nimble/nimble_opt.h"
#include "host/ble_hs.h"
#include "host/ble_gap.h"
#include "host/ble_gatt.h"
#ifdef __cplusplus
extern "C" {
#endif

// Indicates whether device provisioning (e.g., over BLE) is complete
extern bool provisioning_complete;

// Core GAP Initialization
void gap_init(void);
int gap_event_cb(struct ble_gap_event *event, void *arg);
// Start BLE advertising
int start_advertising(void);

// BLE GAP Event Callback
int ble_gap_event(struct ble_gap_event *event, void *arg);

// BLE Sync Callback - called when the BLE stack is synchronized


// BLE Reset Callback - called when the BLE stack is reset
void ble_on_reset(int reason);

// Set the device name for BLE advertising
void set_ble_device_name(const char *name);

#ifdef __cplusplus
}
#endif

#endif // BLUETOOTH_GAP_H
