#include "blue_tooth.h"
#include "esp_nimble_hci.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/ble_hs_adv.h"
#include "host/ble_gap.h"
#include "host/ble_gatt.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "esp_log.h"
#include <string.h>
#include <time.h>
#include "esp_bt.h"



#define TAG "BLE_Scale"

// BLE Configuration
static const char *device_name = "SmartScale";
static uint16_t conn_handle = BLE_HS_CONN_HANDLE_NONE;
static bool is_connected = false;

// GATT Service UUIDs
#define WEIGHT_SERVICE_UUID        0x1820  // Weight Scale Service
#define WEIGHT_MEASUREMENT_UUID    0x2A9D  // Weight Measurement Characteristic
#define BODY_COMPOSITION_UUID      0x181B  // Body Composition Service
#define FAT_PERCENTAGE_UUID        0x2A9A  // Fat Percentage Characteristic

// Characteristic handles
static uint16_t weight_measurement_handle;
static uint16_t fat_percentage_handle;

// Measurement structure
typedef struct __attribute__((packed)) {
    uint8_t flags;          // Measurement units (kg) and timestamp present
    uint16_t weight;        // in 0.01kg units
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
} ble_weight_measurement_t;

// GATT Service Definitions
static const ble_uuid16_t weight_service_uuid = {
    .u = {.type = BLE_UUID_TYPE_16},
    .value = WEIGHT_SERVICE_UUID
};

static const ble_uuid16_t weight_measurement_uuid = {
    .u = {.type = BLE_UUID_TYPE_16},
    .value = WEIGHT_MEASUREMENT_UUID
};

static const ble_uuid16_t body_composition_uuid = {
    .u = {.type = BLE_UUID_TYPE_16},
    .value = BODY_COMPOSITION_UUID
};

static const ble_uuid16_t fat_percentage_uuid = {
    .u = {.type = BLE_UUID_TYPE_16},
    .value = FAT_PERCENTAGE_UUID
};

// Characteristics
static struct ble_gatt_chr_def weight_characteristic = {
    .uuid = &weight_measurement_uuid.u,
    .access_cb = NULL,
    .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
    .val_handle = &weight_measurement_handle
};

static struct ble_gatt_chr_def fat_percentage_characteristic = {
    .uuid = &fat_percentage_uuid.u,
    .access_cb = NULL,
    .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
    .val_handle = &fat_percentage_handle
};


// Characteristics for Weight Service
static struct ble_gatt_chr_def weight_service_chars[] = {
    {
        .uuid = &weight_measurement_uuid.u,
        .access_cb = NULL,
        .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
        .val_handle = &weight_measurement_handle
    },
    { 0 } // End of characteristics
};


// Services
static struct ble_gatt_svc_def gatt_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &weight_service_uuid.u,
        .characteristics = &weight_characteristic,
        .characteristics = NULL
    },
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &body_composition_uuid.u,
        .characteristics = &fat_percentage_characteristic,
        .characteristics = NULL
    },
    { 0 } // End of services
};

// BLE Event Handler
static int ble_gap_event(struct ble_gap_event *event, void *arg) {
    switch (event->type) {
        case BLE_GAP_EVENT_CONNECT:
            if (event->connect.status == 0) {
                ESP_LOGI(TAG, "BLE Connected");
                conn_handle = event->connect.conn_handle;
                is_connected = true;
            } else {
                ESP_LOGE(TAG, "Connection failed: %d", event->connect.status);
            }
            break;

        case BLE_GAP_EVENT_DISCONNECT:
            ESP_LOGI(TAG, "BLE Disconnected (reason: 0x%02x)", event->disconnect.reason);
            is_connected = false;
            conn_handle = BLE_HS_CONN_HANDLE_NONE;
            start_advertising();
            break;

        case BLE_GAP_EVENT_SUBSCRIBE:
            ESP_LOGI(TAG, "Subscribe event: attr_handle=%d",
                   event->subscribe.attr_handle);
            break;
    }
    return 0;
}

 void start_advertising(void) {
    struct ble_gap_adv_params adv_params = {
        .conn_mode = BLE_GAP_CONN_MODE_UND,
        .disc_mode = BLE_GAP_DISC_MODE_GEN
    };

    struct ble_hs_adv_fields fields = {
        .flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP,
        .name = (uint8_t *)device_name,
        .name_len = strlen(device_name),
        .name_is_complete = 1,
        .uuids16 = &weight_service_uuid,
        .num_uuids16 = 1,
        .uuids16_is_complete = 1
    };

    ble_gap_adv_set_fields(&fields);
    ble_gap_adv_start(BLE_OWN_ADDR_RANDOM, NULL, BLE_HS_FOREVER,
                     &adv_params, ble_gap_event, NULL);
}

static void ble_host_sync_cb(void) {
    ble_addr_t addr;
    ble_hs_id_gen_rnd(1, &addr);
    ble_hs_id_set_rnd(addr.val);
    start_advertising();
}

static void ble_host_task_cb(void *param) {
    ESP_LOGI(TAG, "BLE Host Task Started");

    ble_hs_cfg.sync_cb = ble_host_sync_cb;
    ble_hs_cfg.reset_cb = NULL;
    ble_hs_cfg.store_status_cb = NULL;

    ble_svc_gap_init();
    ble_svc_gatt_init();

    ble_gatts_count_cfg(gatt_svcs);
    ble_gatts_add_svcs(gatt_svcs);

    nimble_port_run();
    nimble_port_freertos_deinit();
}

// Public API Implementation
esp_err_t bluetooth_nimble_init(void) {
    ESP_LOGI(TAG, "Initializing BLE");

    // Initialize controller config with defaults
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    
    // Initialize and enable BLE controller
    esp_err_t ret = esp_bt_controller_init(&bt_cfg);
    if (ret != ESP_OK) return ret;
    
    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret != ESP_OK) return ret;

    // Initialize NimBLE stack
    ret = esp_nimble_hci_init();
    if (ret != ESP_OK) return ret;

    nimble_port_init();
    ble_hs_cfg.sync_cb = ble_host_sync_cb;
    nimble_port_freertos_init(ble_host_task_cb);

    return ESP_OK;
}
void bluetooth_update_weight(float weight_kg, bool stable) {
    if (!is_connected) return;

    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);

    ble_weight_measurement_t measurement = {
        .flags = stable ? 0x01 : 0x00,  // Bit 0: Weight is stable
        .weight = (uint16_t)(weight_kg * 100),  // Convert to 0.01kg units
        .year = (uint16_t)(timeinfo.tm_year + 1900),
        .month = (uint8_t)(timeinfo.tm_mon + 1),
        .day = (uint8_t)timeinfo.tm_mday,
        .hour = (uint8_t)timeinfo.tm_hour,
        .minute = (uint8_t)timeinfo.tm_min,
        .second = (uint8_t)timeinfo.tm_sec
    };

    struct os_mbuf *om = ble_hs_mbuf_from_flat(&measurement, sizeof(measurement));
    ble_gatts_notify_custom(conn_handle, weight_measurement_handle, om);
}

void bluetooth_update_fat_percentage(float percentage) {
    if (!is_connected) return;

    uint16_t value = (uint16_t)(percentage * 100);  // Convert to 0.01% units
    struct os_mbuf *om = ble_hs_mbuf_from_flat(&value, sizeof(value));
    ble_gatts_notify_custom(conn_handle, fat_percentage_handle, om);
}

bool bluetooth_is_connected(void) {
    return is_connected;
}

esp_err_t bluetooth_nimble_deinit(void) {
    ESP_LOGI(TAG, "Deinitializing BLE");

    if (is_connected) {
        ble_gap_terminate(conn_handle, BLE_ERR_REM_USER_CONN_TERM);
    }

    nimble_port_freertos_deinit();
    nimble_port_deinit();
    esp_nimble_hci_deinit();

    return ESP_OK;
}