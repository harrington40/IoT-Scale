#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nimble/ble.h"
#include "host/ble_hs.h"
#include "host/ble_gatt.h"
#include "bluetooth_gatt.h"
#include "bluetooth_gap.h"

static const char *TAG = "BLUETOOTH_COMM_GATT";

// Shared variables from bluetooth_gap.h
extern uint16_t conn_handle;
extern bool ble_connected;

// GATT-specific variables
uint16_t custom_chr_handle = 0;
uint8_t received_data[128];
size_t received_data_len = 0;


// Custom UUIDs
static const ble_uuid128_t custom_service_uuid =
    BLE_UUID128_INIT(0xf0, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12, 
                    0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12);

static const ble_uuid128_t custom_char_uuid =
    BLE_UUID128_INIT(0x54, 0x98, 0xcb, 0xde, 0x12, 0x34, 0x56, 0x78, 
                    0x12, 0x34, 0x56, 0x78, 0x12, 0x34, 0x56, 0x78);

// Notification sending function
int custom_send_notification(const uint8_t *data, uint16_t len) {
    if (!enable_notifications) {
        ESP_LOGI(TAG, "Notifications are disabled");
        return 0;
    }

    if (conn_handle == BLE_HS_CONN_HANDLE_NONE) {
        ESP_LOGW(TAG, "No connected client");
        return -1;
    }

    struct os_mbuf *om = ble_hs_mbuf_from_flat(data, len);
    if (!om) {
        ESP_LOGE(TAG, "Failed to allocate mbuf");
        return -1;
    }

    int rc = ble_gatts_notify_custom(conn_handle, custom_chr_handle, om);
    if (rc != 0) {
        ESP_LOGE(TAG, "Notification failed: %d", rc);
        os_mbuf_free_chain(om);
    }
    return rc;
}

// GATT access callback
static int custom_access_cb(uint16_t conn_handle, uint16_t attr_handle,
                          struct ble_gatt_access_ctxt *ctxt, void *arg) {
    switch (ctxt->op) {
        case BLE_GATT_ACCESS_OP_READ_CHR:
            os_mbuf_append(ctxt->om, received_data, received_data_len);
            ESP_LOGI(TAG, "Characteristic read");
            return 0;

        case BLE_GATT_ACCESS_OP_WRITE_CHR:
            received_data_len = OS_MBUF_PKTLEN(ctxt->om);
            os_mbuf_copydata(ctxt->om, 0, received_data_len, received_data);
            ESP_LOGI(TAG, "Received %d bytes", received_data_len);
            
            // Acknowledge receipt
            if (enable_notifications) {
                const char *ack = "ACK";
                custom_send_notification((uint8_t *)ack, strlen(ack));
            }
            return 0;

        default:
            return BLE_ATT_ERR_UNLIKELY;
    }
}

// GATT service definition
static const struct ble_gatt_svc_def gatt_services[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &custom_service_uuid.u,
        .characteristics = (struct ble_gatt_chr_def[]){
            {
                .uuid = &custom_char_uuid.u,
                .access_cb = custom_access_cb,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_NOTIFY,
                .val_handle = &custom_chr_handle,
            },
            {0} // Terminator
        },
    },
    {0} // Terminator
};

// Initialize GATT services
void gatt_init(void) {
    ESP_LOGI(TAG, "Initializing GATT services");
    
    int rc = ble_gatts_count_cfg(gatt_services);
    if (rc != 0) {
        ESP_LOGE(TAG, "Service count failed: %d", rc);
        return;
    }

    rc = ble_gatts_add_svcs(gatt_services);
    if (rc != 0) {
        ESP_LOGE(TAG, "Service add failed: %d", rc);
        return;
    }
    
    ESP_LOGI(TAG, "GATT services initialized");
}

// Public API to send data
void bluetooth_comm_gatt_send_data(const uint8_t *data, size_t len) {
    if (len > sizeof(received_data)) {
        ESP_LOGE(TAG, "Data too large");
        return;
    }
    custom_send_notification(data, len);
}