#ifndef BLUETOOTH_COMMON_H
#define BLUETOOTH_COMMON_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif
extern  bool enable_notifications;
esp_err_t bluetooth_init(void);
void bluetooth_deinit(void);
void bluetooth_send_data(const uint8_t *data, size_t len);
void set_notifications_enabled(bool enabled);
void set_ble_connected(bool status);
bool is_bluetooth_connected(void);
void ble_on_reset(int reason);
bool get_notifications_enabled(void);
 void ble_app_on_sync(void);

#ifdef __cplusplus
}
#endif

#endif // BLUETOOTH_COMMON_H