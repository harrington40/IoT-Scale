#include "bluetooth_common.h"
#include "bluetooth_gap.h"
#include "bluetooth_gatt.h"
#include "esp_nimble_hci.h"
#include "services/gap/ble_svc_gap.h"
#include "host/ble_hs.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "esp_log.h"
#include "esp_bt.h"
#include "freertos/task.h"
#include "nimble/nimble_port.h"
#define BLE_TASK_NAME "nimble_host"
#define BLE_TASK_STACK_SIZE 4096
#define BLE_TASK_PRIORITY 5

static const char *TAG = "BLUETOOTH";
static bool ble_connected = false;
 bool enable_notifications = true;
static uint16_t conn_handle = BLE_HS_CONN_HANDLE_NONE;
static void ble_host_task(void *param);

static void ble_on_sync(void);

/* 128-bit UUIDs (use your own UUIDs) */
static const ble_uuid128_t GATT_SVC_UUID = BLE_UUID128_INIT(0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 
                                                          0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff);
static const ble_uuid128_t GATT_CHR_UUID = BLE_UUID128_INIT(0x01, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                                                          0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff);

/* BLE Host Task */
static void ble_host_task(void *param)
{
    ESP_LOGI(TAG, "BLE Host Task Started");
    nimble_port_run();
    nimble_port_freertos_deinit();
}


void ble_on_sync(void) {
    ESP_LOGI(TAG, "BLE host synced");
    start_advertising();
}

 void ble_on_reset(int reason) {
    ESP_LOGE(TAG, "BLE reset due to reason: %d", reason);
}


/* GATT Characteristic Write Callback */
static int gatt_svc_write_cb(uint16_t conn_handle, uint16_t attr_handle,
                            struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    ESP_LOGI(TAG, "Data received: len=%d", ctxt->om->om_len);
    /* Process received data here */
    return 0;
}

/* Initialize GATT Server */
static void init_gatt_server(void)
{
    struct ble_gatt_chr_def characteristic = {
        .uuid = &GATT_CHR_UUID.u,
        .access_cb = gatt_svc_write_cb,
        .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
    };

    struct ble_gatt_svc_def service = {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &GATT_SVC_UUID.u,
        .characteristics = &characteristic
    };

    /* Terminate the characteristics array */
    struct ble_gatt_chr_def end_chr = { 0 };
    characteristic.val_handle = NULL;
    
    /* Add services */
    ble_gatts_count_cfg((const struct ble_gatt_svc_def[]){ service });
    ble_gatts_add_svcs((const struct ble_gatt_svc_def[]){ service });
}

/* Initialize Bluetooth */
esp_err_t bluetooth_init(void) {
    esp_err_t ret;
    
    // 1. Release Classic BT memory if not needed
    ret = esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to release BT memory: %s", esp_err_to_name(ret));
        return ret;
    }

    // 2. Configure controller with default parameters
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    
    // 3. Initialize controller
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Controller init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // 4. Enable controller in BLE mode
    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Controller enable failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // 5. Initialize NimBLE stack
    ret = nimble_port_init();
    if (ret != 0) {
        ESP_LOGE(TAG, "NimBLE init failed: %d", ret);
        return ESP_FAIL;
    }

    // 6. Configure host settings
    ble_hs_cfg.reset_cb = ble_on_reset;
    ble_hs_cfg.sync_cb = ble_on_sync;
    ble_hs_cfg.store_status_cb = NULL;

    // 7. Initialize GAP and GATT services
    ble_svc_gap_init();
    
    gatt_init();  // Your custom GATT services

    // 8. Set device name
    ble_svc_gap_device_name_set("ESP32-BLE");

    // 9. Start host task
    nimble_port_freertos_init(ble_host_task);

    ESP_LOGI(TAG, "Bluetooth initialized successfully");
    return ESP_OK;
}




/* Send Data via BLE */
void bluetooth_send_data(const uint8_t *data, size_t len)
{
    if (!ble_connected || !enable_notifications || conn_handle == BLE_HS_CONN_HANDLE_NONE) {
        ESP_LOGW(TAG, "Cannot send data - BLE not ready");
        return;
    }

    struct os_mbuf *om = ble_hs_mbuf_from_flat(data, len);
    if (!om) {
        ESP_LOGE(TAG, "Failed to allocate memory for data");
        return;
    }

    /* Get the characteristic value handle */
    uint16_t val_handle;
    int rc = ble_gatts_find_chr(&GATT_SVC_UUID.u, &GATT_CHR_UUID.u, NULL, &val_handle);
    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to find characteristic: %d", rc);
        os_mbuf_free_chain(om);
        return;
    }

    rc = ble_gatts_notify_custom(conn_handle, val_handle, om);
    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to send notification: %d", rc);
        os_mbuf_free_chain(om);
    }
}

/* Deinitialize Bluetooth */
void bluetooth_deinit(void)
{
    nimble_port_stop();
    vTaskDelay(pdMS_TO_TICKS(100));
    nimble_port_freertos_deinit();
    esp_nimble_hci_deinit();
    esp_bt_controller_disable();
    esp_bt_controller_deinit();
    ble_connected = false;
    conn_handle = BLE_HS_CONN_HANDLE_NONE;
}

// Implementation of functions
void set_notifications_enabled(bool enabled) {
    enable_notifications = enabled;
    ESP_LOGI(TAG, "Notifications %s", enabled ? "enabled" : "disabled");
}

bool get_notifications_enabled(void) {
    return enable_notifications;
}

