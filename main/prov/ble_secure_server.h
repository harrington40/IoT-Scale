#ifndef BLE_SECURE_SERVER_H
#define BLE_SECURE_SERVER_H
#include "esp_gap_ble_api.h"      // ✅ Defines esp_gap_ble_cb_event_t and esp_ble_gap_cb_param_t
#include "esp_gatts_api.h"   


void ble_secure_server_gatts_cb(esp_gatts_cb_event_t event,
                                esp_gatt_if_t gatts_if,
                                esp_ble_gatts_cb_param_t *param);

void ble_secure_server_gap_cb(esp_gap_ble_cb_event_t event,
                              esp_ble_gap_cb_param_t *param);
                              void trigger_ota_mode(void); 
                            
void start_wifi_for_ota(void);
esp_err_t start_https_ota_update(void);

#endif // BLE_SECURE_SERVER_H
