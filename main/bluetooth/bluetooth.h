#ifndef BLUETOOTH_H
#define BLUETOOTH_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "services/gap/ble_svc_gap.h"
#include "esp_log.h"

#ifdef __cplusplus
extern "C" {
#endif

// Indicates whether device provisioning (e.g. over BLE) is complete
extern bool provisioning_complete;

// Core BLE initialization & advertising
void ble_init(void);
void start_advertising(void);

// Application-level callback invoked once controller and host are synced
void ble_app_on_sync(void);

// Send raw bytes over an established BLE characteristic
void bluetooth_send_data(const uint8_t *data, size_t len);

// Low-level callbacks (registered with the NimBLE host)
void ble_on_reset(int reason);
void ble_on_sync(int status);
void gatt_svr_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg);

#ifdef __cplusplus
}
#endif

#endif // BLUETOOTH_H
