#include "bluetooth_gap.h"
#include "esp_log.h"

static const char *TAG = "BLUETOOTH_GAP";

// Shared connection state
uint16_t conn_handle = BLE_HS_CONN_HANDLE_NONE;
bool ble_connected = false;

// GAP event callback
int gap_event_cb(struct ble_gap_event *event, void *arg) {
    switch (event->type) {
        case BLE_GAP_EVENT_CONNECT:
            conn_handle = event->connect.conn_handle;
            ble_connected = true;
            ESP_LOGI(TAG, "Connected");
            break;

        case BLE_GAP_EVENT_DISCONNECT:
            ble_connected = false;
            conn_handle = BLE_HS_CONN_HANDLE_NONE;
            ESP_LOGI(TAG, "Disconnected");
            start_advertising();
            break;

        default:
            break;
    }
    return 0;
}

// Advertising function
int start_advertising(void) {
    struct ble_gap_adv_params adv_params = {
        .conn_mode = BLE_GAP_CONN_MODE_UND,
        .disc_mode = BLE_GAP_DISC_MODE_GEN
    };

    uint8_t adv_data[] = {
        0x02, 0x01, 0x06,       // Flags
        0x03, 0x03, 0xAB, 0xCD, // Service UUID
        0x0A, 0x09, 'E','S','P','3','2','-','G','A','T','T' // Name
    };

    int rc = ble_gap_adv_set_data(adv_data, sizeof(adv_data));
    if (rc != 0) return rc;

    return ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, NULL, BLE_HS_FOREVER,
                           &adv_params, gap_event_cb, NULL);
}