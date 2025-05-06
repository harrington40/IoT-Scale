
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

// NimBLE headers
#include "nimble/ble.h"
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "host/ble_gap.h"
#include "host/ble_gatt.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "os/queue.h"
#include "host/ble_hs_mbuf.h"  // For creating mbufs
#include "bluetooth.h"
#include "bluetooth_comm_gatt.h"

// Logging tag
 static const char *TAG = "CUSTOM_GATT";

// Global variable to track the current connection handle
uint16_t current_conn_handle = BLE_HS_CONN_HANDLE_NONE;

// Global variable for the characteristic handle
uint16_t custom_chr_handle = 4; // Replace with dynamically obtained handle if possible

// Buffer to store data received from the client
uint8_t received_data[128] = {0};
size_t received_data_len = 0;

// Global flag to enable/disable notifications at runtime
static bool enable_notifications = true; // Set to false to disable notifications

// Custom Service UUID: f0debc9a-7856-3412-7856-341278563412
static const ble_uuid128_t custom_service_uuid =
    BLE_UUID128_INIT(0xf0, 0xde, 0xbc, 0x9a,
                     0x78, 0x56,
                     0x34, 0x12,
                     0x78, 0x56,
                     0x34, 0x12,
                     0x78, 0x56,
                     0x34, 0x12);

// Custom Characteristic UUID: 5498cbde-1234-5678-1234-567812345678
static const ble_uuid128_t custom_char_uuid =
    BLE_UUID128_INIT(0x54, 0x98, 0xcb, 0xde,
                     0x12, 0x34,
                     0x56, 0x78,
                     0x12, 0x34,
                     0x56, 0x78,
                     0x12, 0x34,
                     0x56, 0x78);

// Access callback for the custom characteristic
static int custom_access_cb(uint16_t conn_handle, uint16_t attr_handle,
                            struct ble_gatt_access_ctxt *ctxt, void *arg) {
    switch (ctxt->op) {
    case BLE_GATT_ACCESS_OP_READ_CHR: {
        // Respond with the stored data
        os_mbuf_append(ctxt->om, received_data, received_data_len);
        ESP_LOGI(TAG, "Read request: sending response -> %.*s", received_data_len, received_data);
        return 0;
    }
    case BLE_GATT_ACCESS_OP_WRITE_CHR: {
        // Store the received data
        received_data_len = OS_MBUF_PKTLEN(ctxt->om);
        os_mbuf_copydata(ctxt->om, 0, received_data_len, received_data);
        ESP_LOGI(TAG, "Write request: received -> %.*s", received_data_len, received_data);

        // Optionally, send a notification in response to the write
#if ENABLE_NOTIFICATIONS
        if (enable_notifications) {
            const char *notify_msg = "Data received";
            custom_send_notification((const uint8_t *)notify_msg, strlen(notify_msg));
        }
#endif
        return 0;
    }
    default:
        return BLE_ATT_ERR_UNLIKELY;
    }
}

// Define the custom GATT service and its characteristic
static const struct ble_gatt_svc_def gatt_services[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = (const ble_uuid_t *)&custom_service_uuid.u,
        .characteristics = (struct ble_gatt_chr_def[]){
            {
                .uuid = (const ble_uuid_t *)&custom_char_uuid.u,
                .access_cb = custom_access_cb,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_NOTIFY,
                .val_handle = &custom_chr_handle, // This field can be set by the stack if supported.
            },
            { 0 } // End of characteristics.
        },
    },
    { 0 } // End of services.
};

// GAP event callback to handle connection and disconnection
int ble_gap_event(struct ble_gap_event *event, void *arg) {
    switch (event->type) {
    case BLE_GAP_EVENT_CONNECT:
        if (event->connect.status == 0) {
            current_conn_handle = event->connect.conn_handle;
            ESP_LOGI(TAG, "Client connected; conn_handle=%d", current_conn_handle);
        } else {
            ESP_LOGE(TAG, "Connection failed; status=%d", event->connect.status);
        }
        break;

    case BLE_GAP_EVENT_DISCONNECT:
        ESP_LOGI(TAG, "Client disconnected; conn_handle=%d", event->disconnect.conn.conn_handle);
        current_conn_handle = BLE_HS_CONN_HANDLE_NONE;
        // Restart advertising after a disconnect.
        {
            struct ble_gap_adv_params adv_params = {0};
            int rc = ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, NULL, BLE_HS_FOREVER, &adv_params, NULL, NULL);
            if (rc != 0) {
                ESP_LOGE(TAG, "Error restarting advertisement; rc=%d", rc);
            }
        }
        break;

    default:
        break;
    }
    return 0;
}

// Function to send a notification on the custom characteristic
int custom_send_notification(const uint8_t *data, uint16_t len) {
#if ENABLE_NOTIFICATIONS
    if (!enable_notifications) {
        ESP_LOGI(TAG, "Notifications are disabled; skipping notification");
        return 0; // Return success even if notifications are disabled
    }

    if (current_conn_handle == BLE_HS_CONN_HANDLE_NONE) {
        ESP_LOGW(TAG, "No connected client; cannot send notification");
        return -1;
    }
    struct os_mbuf *om = ble_hs_mbuf_from_flat(data, len);
    if (!om) {
        ESP_LOGE(TAG, "Failed to allocate mbuf for notification");
        return -1;
    }
    int rc = ble_gatts_notify_custom(current_conn_handle, custom_chr_handle, om);
    if (rc != 0) {
        ESP_LOGE(TAG, "Notification failed; rc=%d", rc);
    } else {
        ESP_LOGI(TAG, "Notification sent: %.*s", len, data);
    }
    return rc;
#else
    ESP_LOGI(TAG, "Notifications are disabled at compile time; skipping notification");
    return 0; // Return success even if notifications are disabled
#endif
}

// Function to enable/disable notifications at runtime
void set_notifications_enabled(bool enabled) {
    enable_notifications = enabled;
    ESP_LOGI(TAG, "Notifications %s", enabled ? "enabled" : "disabled");
}

// Callback executed when the NimBLE host is synchronized

// BLE host task
void ble_host_task(void *param) {
    nimble_port_run();
    nimble_port_freertos_deinit();
}

// Initialize the custom GATT service
void custom_gatt_init(void) {
    ESP_LOGI(TAG, "Initializing NimBLE");

    nimble_port_init();
    // Set synchronization callback (invoked when the BLE stack is ready).
    ble_hs_cfg.sync_cb = ble_app_on_sync;

    int rc = ble_gatts_count_cfg(gatt_services);
    if (rc != 0) {
        ESP_LOGE(TAG, "ble_gatts_count_cfg() failed; rc=%d", rc);
        return;
    }
    rc = ble_gatts_add_svcs(gatt_services);
    if (rc != 0) {
        ESP_LOGE(TAG, "ble_gatts_add_svcs() failed; rc=%d", rc);
        return;
    }
    ESP_LOGI(TAG, "Assuming characteristic handle is %d", custom_chr_handle);

    // Start the BLE host task.
    nimble_port_freertos_init(ble_host_task);
}

