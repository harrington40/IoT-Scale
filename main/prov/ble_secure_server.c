#include "ble_secure_server.h"
#include "esp_log.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"
#include "esp_gap_ble_api.h"
#include "string.h"

#define TAG "BLE_SECURE_SERVER"

/// Constants
#define DEVICE_NAME "SecureScaleBLE"

static const uint16_t GATTS_SERVICE_UUID        = 0x00FF;
static const uint16_t GATTS_CHAR_UUID_A         = 0xFF01;
static const uint16_t GATTS_CHAR_UUID_B         = 0xFF02;
static const uint16_t GATTS_CHAR_UUID_C         = 0xFF03;
static const uint16_t BATTERY_SERVICE_UUID      = 0x180F;
static const uint16_t BATTERY_LEVEL_UUID        = 0x2A19;
static const uint16_t COMMAND_SERVICE_UUID      = 0xFFF0;
static const uint16_t COMMAND_CHAR_UUID         = 0xFFF1;

/// Attribute table index
// Secure Service
enum {
    SECURE_IDX_SERVICE,
    SECURE_IDX_CHAR_A,
    SECURE_IDX_CHAR_VAL_A,
    SECURE_IDX_CHAR_CFG_A,
    SECURE_IDX_CHAR_B,
    SECURE_IDX_CHAR_VAL_B,
    SECURE_IDX_CHAR_C,
    SECURE_IDX_CHAR_VAL_C,
    SECURE_IDX_NB,
};

// Battery Service
enum {
    BATTERY_IDX_SERVICE,
    BATTERY_IDX_CHAR,
    BATTERY_IDX_VAL,
    BATTERY_IDX_NB,
};

// Command Service
enum {
    COMMAND_IDX_SERVICE,
    COMMAND_IDX_CHAR,
    COMMAND_IDX_VAL,
    COMMAND_IDX_NB,
};
#define SECURE_SVC_INST_ID   0
#define BATTERY_SVC_INST_ID  1
#define COMMAND_SVC_INST_ID  2

static uint16_t secure_handle_table[SECURE_IDX_NB];

/// Dummy values
static uint8_t char_a_value[4] = {0x01, 0x02, 0x03, 0x04};
static uint8_t char_b_value[2] = {0x05, 0x06};
static uint8_t char_c_value[1] = {0x07};
static uint8_t battery_level = 95;
static uint8_t command_value[20] = {0};



uint16_t battery_handle_table[BATTERY_IDX_NB];
uint16_t command_handle_table[COMMAND_IDX_NB];
/// UUIDs
static const uint16_t primary_service_uuid = ESP_GATT_UUID_PRI_SERVICE;
static const uint16_t char_decl_uuid = ESP_GATT_UUID_CHAR_DECLARE;
static const uint16_t char_client_cfg_uuid = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;

static const uint8_t prop_read_notify = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
static const uint8_t prop_read = ESP_GATT_CHAR_PROP_BIT_READ;
static const uint8_t prop_rw = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE;

static esp_gatts_attr_db_t secure_svc_db[SECURE_IDX_NB] = {
    [SECURE_IDX_SERVICE] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&primary_service_uuid, ESP_GATT_PERM_READ,
      sizeof(uint16_t), sizeof(uint16_t), (uint8_t *)&GATTS_SERVICE_UUID}},

    [SECURE_IDX_CHAR_A] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&char_decl_uuid, ESP_GATT_PERM_READ,
      sizeof(uint8_t), sizeof(uint8_t), (uint8_t *)&prop_read_notify}},

    [SECURE_IDX_CHAR_VAL_A] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_A, ESP_GATT_PERM_READ,
      sizeof(char_a_value), sizeof(char_a_value), char_a_value}},

    [SECURE_IDX_CHAR_CFG_A] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&char_client_cfg_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      sizeof(uint16_t), 0, NULL}},

    [SECURE_IDX_CHAR_B] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&char_decl_uuid, ESP_GATT_PERM_READ,
      sizeof(uint8_t), sizeof(uint8_t), (uint8_t *)&prop_read}},

    [SECURE_IDX_CHAR_VAL_B] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_B, ESP_GATT_PERM_READ,
      sizeof(char_b_value), sizeof(char_b_value), char_b_value}},

    [SECURE_IDX_CHAR_C] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&char_decl_uuid, ESP_GATT_PERM_READ,
      sizeof(uint8_t), sizeof(uint8_t), (uint8_t *)&prop_read}},

    [SECURE_IDX_CHAR_VAL_C] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_C, ESP_GATT_PERM_READ,
      sizeof(char_c_value), sizeof(char_c_value), char_c_value}},
};
 


static esp_gatts_attr_db_t battery_svc_db[BATTERY_IDX_NB] = {
    [BATTERY_IDX_SERVICE] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&primary_service_uuid, ESP_GATT_PERM_READ,
      sizeof(uint16_t), sizeof(uint16_t), (uint8_t *)&BATTERY_SERVICE_UUID}},

    [BATTERY_IDX_CHAR] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&char_decl_uuid, ESP_GATT_PERM_READ,
      sizeof(uint8_t), sizeof(uint8_t), (uint8_t *)&prop_read}},

    [BATTERY_IDX_VAL] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&BATTERY_LEVEL_UUID, ESP_GATT_PERM_READ,
      sizeof(uint8_t), sizeof(uint8_t), &battery_level}},
};

static esp_gatts_attr_db_t command_svc_db[COMMAND_IDX_NB] = {
    [COMMAND_IDX_SERVICE] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&primary_service_uuid, ESP_GATT_PERM_READ,
      sizeof(uint16_t), sizeof(uint16_t), (uint8_t *)&COMMAND_SERVICE_UUID}},

    [COMMAND_IDX_CHAR] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&char_decl_uuid, ESP_GATT_PERM_READ,
      sizeof(uint8_t), sizeof(uint8_t), (uint8_t *)&prop_rw}},

    [COMMAND_IDX_VAL] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&COMMAND_CHAR_UUID, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      sizeof(command_value), sizeof(command_value), command_value}},
};



static esp_ble_adv_params_t adv_params = {
    .adv_int_min        = 0x20,
    .adv_int_max        = 0x40,
    .adv_type           = ADV_TYPE_IND,
    .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
    .channel_map        = ADV_CHNL_ALL,
    .adv_filter_policy  = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

void ble_secure_server_gatts_cb(esp_gatts_cb_event_t event,
                                esp_gatt_if_t gatts_if,
                                esp_ble_gatts_cb_param_t *param) {
    switch (event) {
    case ESP_GATTS_REG_EVT:
        ESP_LOGI(TAG, "Register App ID");
        esp_ble_gap_set_device_name(DEVICE_NAME);

        uint8_t service_uuid[] = {0xFF, 0x00};
        esp_ble_adv_data_t adv_data = {
            .set_scan_rsp = false,
            .include_name = true,
            .include_txpower = true,
            .min_interval = 0x0006,
            .max_interval = 0x0010,
            .appearance = 0x00,
            .manufacturer_len = 0,
            .p_manufacturer_data = NULL,
            .service_data_len = 0,
            .p_service_data = NULL,
            .service_uuid_len = sizeof(service_uuid),
            .p_service_uuid = service_uuid,
            .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
        };
        esp_ble_gap_config_adv_data(&adv_data);

        uint8_t scan_rsp[] = {'Q','R',':','S','M','A','R','T','-','S','C','A','L','E'};
        esp_ble_gap_config_scan_rsp_data_raw(scan_rsp, sizeof(scan_rsp));

        esp_ble_gatts_create_attr_tab(secure_svc_db, gatts_if, SECURE_IDX_NB, SECURE_SVC_INST_ID);
esp_ble_gatts_create_attr_tab(battery_svc_db, gatts_if, BATTERY_IDX_NB, BATTERY_SVC_INST_ID);
esp_ble_gatts_create_attr_tab(command_svc_db, gatts_if, COMMAND_IDX_NB, COMMAND_SVC_INST_ID);
       // esp_ble_gap_start_advertising(&adv_params);
        break;

  case ESP_GATTS_CREAT_ATTR_TAB_EVT:
    if (param->add_attr_tab.status != ESP_GATT_OK) {
        ESP_LOGE(TAG, "Create attr table failed, error 0x%x", param->add_attr_tab.status);
        break;
    }

    if (param->add_attr_tab.svc_inst_id == SECURE_SVC_INST_ID) {
        memcpy(secure_handle_table, param->add_attr_tab.handles, sizeof(secure_handle_table));
        esp_ble_gatts_start_service(secure_handle_table[SECURE_IDX_SERVICE]);
    } else if (param->add_attr_tab.svc_inst_id == BATTERY_SVC_INST_ID) {
        memcpy(battery_handle_table, param->add_attr_tab.handles, sizeof(battery_handle_table));
        esp_ble_gatts_start_service(battery_handle_table[BATTERY_IDX_SERVICE]);
    } else if (param->add_attr_tab.svc_inst_id == COMMAND_SVC_INST_ID) {
        memcpy(command_handle_table, param->add_attr_tab.handles, sizeof(command_handle_table));
        esp_ble_gatts_start_service(command_handle_table[COMMAND_IDX_SERVICE]);
    }
    break;
    case ESP_GATTS_READ_EVT:
        ESP_LOGI(TAG, "Read event");
        if (param->read.handle == secure_handle_table[SECURE_IDX_CHAR_VAL_A]) {
   esp_err_t esp_ble_gatts_send_response(    
    esp_gatt_if_t gatts_if,
    uint16_t conn_id,
    uint32_t trans_id,
    esp_gatt_status_t status,
    esp_gatt_rsp_t *rsp );


    default:
        break;
    }
}
                                }

void ble_secure_server_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    // Handle GAP events
    switch (event) {
        case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
            ESP_LOGI("BLE", "Advertising started");
            break;
        case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
            ESP_LOGI("BLE", "Advertising stopped");
            break;
        // Add more events as needed
        default:
            ESP_LOGI("BLE", "GAP event: %d", event);
            break;
    }
  }
