#include "ble_secure_server.h"
#include "esp_log.h"
#include "esp_gatts_api.h"
#include "esp_gap_ble_api.h"
#include "esp_bt_main.h"
#include "esp_bt_defs.h"
#include "esp_bt_device.h"
#include "esp_err.h"
#include "esp_bt.h"


#define MAX_VALUE_LEN 512 

static const char *TAG = "BLE_SEC_SERVER";

/* UUIDs */
static const uint16_t primary_service_uuid       = ESP_GATT_UUID_PRI_SERVICE;
static const uint16_t character_declaration_uuid = ESP_GATT_UUID_CHAR_DECLARE;
static const uint16_t client_characteristic_config_uuid = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;

/* Service UUID */
static const uint16_t SERVICE_UUID = 0x00FF;

/* 128-bit Characteristic UUIDs */
static const uint8_t CHAR_UUID_A[16] = {
    0xA1, 0xB2, 0xC3, 0xD4, 0xE5, 0xF6, 0xA7, 0xB8,
    0xC9, 0xDA, 0xEB, 0xFC, 0x0D, 0x1E, 0x2F, 0x30
};
static const uint8_t CHAR_UUID_B[16] = {
    0xB1, 0xC2, 0xD3, 0xE4, 0xF5, 0xA6, 0xB7, 0xC8,
    0xD9, 0xEA, 0xFB, 0x0C, 0x1D, 0x2E, 0x3F, 0x40
};
static const uint8_t CHAR_UUID_C[16] = {
    0xC1, 0xD2, 0xE3, 0xF4, 0xA5, 0xB6, 0xC7, 0xD8,
    0xE9, 0xFA, 0x0B, 0x1C, 0x2D, 0x3E, 0x4F, 0x50
};

/* Characteristic properties */
static const uint8_t char_prop_read_notify = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
static const uint8_t char_prop_write       = ESP_GATT_CHAR_PROP_BIT_WRITE;
static const uint8_t char_prop_read_write  = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE;

/* Default notify value (disabled) */
static uint8_t notify_en = 0;

/* Application Profile ID */
#define PROFILE_APP_ID 0

/* Attribute Table */
static const esp_gatts_attr_db_t gatt_db[HRS_IDX_NB] = {
    [IDX_SVC] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&primary_service_uuid,
        ESP_GATT_PERM_READ, sizeof(uint16_t), sizeof(SERVICE_UUID), (uint8_t *)&SERVICE_UUID}},

    [IDX_CHAR_A] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid,
        ESP_GATT_PERM_READ, sizeof(uint8_t), sizeof(uint8_t), (uint8_t *)&char_prop_read_notify}},
    [IDX_CHAR_VAL_A] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_128, (uint8_t *)CHAR_UUID_A,
        ESP_GATT_PERM_READ, MAX_VALUE_LEN, 0, NULL}},
    [IDX_CHAR_CFG_A] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&client_characteristic_config_uuid,
        ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, sizeof(uint16_t), sizeof(uint16_t), (uint8_t *)&notify_en}},

    [IDX_CHAR_B] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid,
        ESP_GATT_PERM_READ, sizeof(uint8_t), sizeof(uint8_t), (uint8_t *)&char_prop_write}},
    [IDX_CHAR_VAL_B] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_128, (uint8_t *)CHAR_UUID_B,
        ESP_GATT_PERM_WRITE, MAX_VALUE_LEN, 0, NULL}},

    [IDX_CHAR_C] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid,
        ESP_GATT_PERM_READ, sizeof(uint8_t), sizeof(uint8_t), (uint8_t *)&char_prop_read_write}},
    [IDX_CHAR_VAL_C] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_128, (uint8_t *)CHAR_UUID_C,
        ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, MAX_VALUE_LEN, 0, NULL}},
};

/* Placeholder handlers */
void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    ESP_LOGI(TAG, "GAP event: %d", event);
}

void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if,
                         esp_ble_gatts_cb_param_t *param)
{
    ESP_LOGI(TAG, "GATTS event: %d", event);

    switch (event) {
    case ESP_GATTS_REG_EVT:
        ESP_LOGI(TAG, "Register app, setting device name and creating service");
        esp_ble_gap_set_device_name("SecureGATTDevice");
        esp_ble_gatts_create_attr_tab(gatt_db, gatts_if, HRS_IDX_NB, 0);
        break;
    case ESP_GATTS_CREAT_ATTR_TAB_EVT:
        if (param->add_attr_tab.status == ESP_GATT_OK) {
            esp_ble_gatts_start_service(param->add_attr_tab.handles[IDX_SVC]);
        }
        break;
    default:
        break;
    }
}

void ble_secure_server_init(void)
{
    esp_err_t ret;

    ESP_LOGI(TAG, "Initializing BLE Secure GATT Server");

    esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    ESP_ERROR_CHECK(ret);

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    ESP_ERROR_CHECK(ret);

    ret = esp_bluedroid_init();
    ESP_ERROR_CHECK(ret);

    ret = esp_bluedroid_enable();
    ESP_ERROR_CHECK(ret);

    ret = esp_ble_gap_register_callback(gap_event_handler);
    ESP_ERROR_CHECK(ret);

    ret = esp_ble_gatts_register_callback(gatts_event_handler);
    ESP_ERROR_CHECK(ret);

    ret = esp_ble_gatts_app_register(PROFILE_APP_ID);
    ESP_ERROR_CHECK(ret);

    // Enable bonding, MITM and secure connections
    esp_ble_auth_req_t auth_req = ESP_LE_AUTH_REQ_SC_MITM_BOND;
    esp_ble_io_cap_t iocap = ESP_IO_CAP_IO;
    uint8_t key_size = 16;

    esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req, sizeof(auth_req));
    esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, sizeof(iocap));
    esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size, sizeof(key_size));
}
