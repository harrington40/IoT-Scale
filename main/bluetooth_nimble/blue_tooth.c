#include "blue_tooth.h"
#include "esp_nimble_hci.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/ble_hs_adv.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "esp_log.h"
#include <string.h>
#include "esp_nimble_hci.h"

#define TAG "NimBLE"

static bool is_connected = false;

// Connection callbacks
static int ble_gap_event_cb(struct ble_gap_event *event, void *arg) {
    switch (event->type) {
        case BLE_GAP_EVENT_CONNECT:
            if (event->connect.status == 0) {
                ESP_LOGI(TAG, "Device connected");
                is_connected = true;
            } else {
                ESP_LOGW(TAG, "Connection failed, restarting adv");
                ble_gap_adv_start(BLE_OWN_ADDR_RANDOM, NULL, BLE_HS_FOREVER, NULL, ble_gap_event_cb, NULL);
            }
            break;

        case BLE_GAP_EVENT_DISCONNECT:
            ESP_LOGI(TAG, "Device disconnected");
            is_connected = false;
            ble_gap_adv_start(BLE_OWN_ADDR_RANDOM, NULL, BLE_HS_FOREVER, NULL, ble_gap_event_cb, NULL);
            break;

        case BLE_GAP_EVENT_ADV_COMPLETE:
            ESP_LOGI(TAG, "Advertising complete; restarting");
            ble_gap_adv_start(BLE_OWN_ADDR_RANDOM, NULL, BLE_HS_FOREVER, NULL, ble_gap_event_cb, NULL);
            break;

        default:
            break;
    }
    return 0;
}

static void ble_host_sync_cb(void) {
    ESP_LOGI(TAG, "BLE Host Sync");

    struct ble_hs_adv_fields fields = {0};
    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    fields.name = (uint8_t *)"SmartScale";
    fields.name_len = strlen("SmartScale");
    fields.name_is_complete = 1;

    int rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0) {
        ESP_LOGE(TAG, "Adv fields set failed: %d", rc);
        return;
    }

    rc = ble_gap_adv_start(BLE_OWN_ADDR_RANDOM, NULL, BLE_HS_FOREVER, NULL, ble_gap_event_cb, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to start advertising: %d", rc);
    }
}

static void ble_host_reset_cb(int reason) {
    ESP_LOGW(TAG, "BLE host reset, reason: %d", reason);
}

bool bluetooth_is_connected(void) {
    return is_connected;
}

static void ble_host_task_cb(void *param) {
    ESP_LOGI(TAG, "BLE Host Task Started");

    // Configure NimBLE host callbacks
    ble_hs_cfg.sync_cb = ble_host_sync_cb;
    ble_hs_cfg.reset_cb = ble_host_reset_cb;

    // Register GATT services
    ble_svc_gap_init();
    ble_svc_gatt_init();

    // Start host
    nimble_port_run();
    nimble_port_freertos_deinit();
}

esp_err_t bluetooth_nimble_init(void) {
    ESP_LOGI(TAG, "Initializing NimBLE");

    if (esp_nimble_hci_init() != ESP_OK) {
        ESP_LOGE(TAG, "Controller init failed");
        return ESP_FAIL;
    }

    if (nimble_port_init() != 0) {
        ESP_LOGE(TAG, "NimBLE port init failed");
        return ESP_FAIL;
    }

    nimble_port_freertos_init(ble_host_task_cb);
    return ESP_OK;
}
