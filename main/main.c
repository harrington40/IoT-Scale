#include <stdio.h>
#include <inttypes.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"

#include "log_utils.h"
#include "wifi_https_server.h"
#include "wifi/wifi_comm.h"
#include "hx711.h"
#include "driver/gpio.h"
#include "weight_manager.h"
#include "sntp_synch.h"
#include "calibration.h"
#include "ble_secure_server.h"

#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"

// Configuration
#define WIFI_SSID           "Tori_2.44Ghz"
#define WIFI_PASS           "Logarithmses900"
#define HX711_DOUT_PIN      GPIO_NUM_4
#define HX711_SCK_PIN       GPIO_NUM_22
#define HX711_GAIN          HX711_GAIN_128
#define MA_WINDOW           8
#define USE_KF              true
#define KF_Q_INIT           0.5f
#define KF_R_INIT           1.0f
#define CAL_SAMPLES         10
#define CAL_KNOWN_WEIGHT_G  200.0f
#define BT_TASK_PRIORITY    (configMAX_PRIORITIES - 3)
#define BT_TASK_STACK_SIZE  (4096)
#define WEIGHT_UPDATE_MS    (200)

static const char *TAG = "app_main";

// Global objects
hx711_t g_scale;
calibration_t g_calib;
httpd_handle_t https_server = NULL;
extern QueueHandle_t g_weight_queue;

void initialize_nvs(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Release memory for Classic BT
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
}

bool https_server_is_running(void) {
    return (https_server != NULL);
}

void app_main(void) {
    LOG_ROW(TAG, "=== Smart-Scale FW starting ===");

    // 1. Initialize NVS and release Classic BT memory
    initialize_nvs();

    // 2. Initialize BLE stack
    LOG_ROW(TAG, "Initializing Bluetooth LE stack...");

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));
    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_BLE));

    ESP_ERROR_CHECK(esp_bluedroid_init());
    ESP_ERROR_CHECK(esp_bluedroid_enable());

    // Register BLE GATT and GAP callbacks
    ESP_ERROR_CHECK(esp_ble_gatts_register_callback(ble_secure_server_gatts_cb));
    ESP_ERROR_CHECK(esp_ble_gap_register_callback(ble_secure_server_gap_cb));
    ESP_ERROR_CHECK(esp_ble_gatts_app_register(0));  // Application ID 0

    // 3. Calibration
    {
        int32_t raw_buf[CAL_SAMPLES];
        int32_t zero_raw = 0;
        int32_t raw_at_weight = 0;

        hx711_init(&g_scale, HX711_DOUT_PIN, HX711_SCK_PIN, HX711_GAIN,
                  MA_WINDOW, USE_KF, KF_Q_INIT, KF_R_INIT);

        for (int i = 0; i < CAL_SAMPLES; i++) {
            raw_buf[i] = hx711_read_raw(&g_scale);
            vTaskDelay(pdMS_TO_TICKS(100));
        }

        calibration_tare(&zero_raw, raw_buf, CAL_SAMPLES);
        LOG_ROW(TAG, "Tare complete, zero_raw = %" PRId32, zero_raw);

        LOG_ROW(TAG, "Place %.0f g known weight on scale...", CAL_KNOWN_WEIGHT_G);
        vTaskDelay(pdMS_TO_TICKS(5000));

        raw_at_weight = hx711_read_raw(&g_scale);
        LOG_ROW(TAG, "Raw at weight = %" PRId32, raw_at_weight);

        calibration_compute(&g_calib, zero_raw, raw_at_weight, CAL_KNOWN_WEIGHT_G);
        LOG_ROW(TAG, "Calibration: slope=%.6f intercept=%.2f",
                g_calib.slope, g_calib.intercept);
    }

    // 4. Initialize Wi-Fi
    wifi_comm_start(WIFI_SSID, WIFI_PASS);

    // 5. HX711 RTOS driver
    hx711_init_rtos(&g_scale, HX711_DOUT_PIN, HX711_SCK_PIN, HX711_GAIN,
                   MA_WINDOW, USE_KF, KF_Q_INIT, KF_R_INIT, 5);

    // 6. Weight manager
    weight_manager_init(hx711_get_queue(), &g_calib);

    // 7. Start HTTPS server if Wi-Fi is up
    int retries = 0;
    while (!wifi_is_connected() && retries++ < 20) {
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    if (wifi_is_connected()) {
        LOG_ROW(TAG, "Starting HTTPS server...");
        init_https_server();
    } else {
        LOG_ROW(TAG, "Wi-Fi not connected, skipping HTTPS server start");
    }

    // Main loop can remain empty or monitor as needed
}
