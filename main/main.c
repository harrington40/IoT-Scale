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
#define WEIGHT_UPDATE_MS    200
#define MAX_RETRIES         5
#define WIFI_CONNECT_TIMEOUT_MS (20 * 1000)

static const char *TAG = "app_main";

// Global objects
static hx711_t g_scale;
static calibration_t g_calib;

void initialize_nvs(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
}

void initialize_bluetooth(void) {
    LOG_ROW(TAG, "Initializing Bluetooth LE stack...");
    
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));
    
    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_BLE));
    ESP_ERROR_CHECK(esp_bluedroid_init());
    ESP_ERROR_CHECK(esp_bluedroid_enable());

    // Register BLE callbacks
    ESP_ERROR_CHECK(esp_ble_gatts_register_callback(ble_secure_server_gatts_cb));
    ESP_ERROR_CHECK(esp_ble_gap_register_callback(ble_secure_server_gap_cb));
    ESP_ERROR_CHECK(esp_ble_gatts_app_register(0));
}

void perform_calibration(void) {
    LOG_ROW(TAG, "Starting calibration procedure...");
    
    int32_t raw_buf[CAL_SAMPLES] = {0};
    int32_t zero_raw = 0;
    int32_t raw_at_weight = 0;

    // Initialize HX711 in blocking mode for calibration
    ESP_ERROR_CHECK(hx711_init(&g_scale, HX711_DOUT_PIN, HX711_SCK_PIN, HX711_GAIN));

    // Take tare measurements
    LOG_ROW(TAG, "Taking %d tare measurements...", CAL_SAMPLES);
    for (int i = 0; i < CAL_SAMPLES; i++) {
        if (hx711_read_raw(&g_scale, &raw_buf[i]) != ESP_OK) {
            LOG_ROW(TAG, "Failed to read raw value %d", i);
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    // Calculate zero point
    calibration_tare(&zero_raw, raw_buf, CAL_SAMPLES);
    LOG_ROW(TAG, "Tare complete, zero_raw = %" PRId32, zero_raw);

    // Take measurement with known weight
    LOG_ROW(TAG, "Place %.0f g known weight on scale...", CAL_KNOWN_WEIGHT_G);
    vTaskDelay(pdMS_TO_TICKS(5000));

    if (hx711_read_raw(&g_scale, &raw_at_weight) != ESP_OK) {
        LOG_ROW(TAG, "Failed to read weight measurement");
        return;
    }
    LOG_ROW(TAG, "Raw at weight = %" PRId32, raw_at_weight);

    // Calculate calibration parameters
    calibration_compute(&g_calib, zero_raw, raw_at_weight, CAL_KNOWN_WEIGHT_G);
    LOG_ROW(TAG, "Calibration complete: slope=%.6f intercept=%.2f",
            g_calib.slope, g_calib.intercept);
}

void start_network_services(void) {
    // Initialize Wi-Fi
    wifi_comm_start(WIFI_SSID, WIFI_PASS);

    // Wait for Wi-Fi connection with timeout
    int retries = 0;
    while (!wifi_is_connected() && retries++ < (WIFI_CONNECT_TIMEOUT_MS/500)) {
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    if (wifi_is_connected()) {
        LOG_ROW(TAG, "Wi-Fi connected, starting network services...");
        
        // Initialize SNTP for time synchronization

        
        // Start HTTPS server
        esp_err_t ret = init_https_server();
        if (ret == ESP_OK) {
            LOG_ROW(TAG, "HTTPS server started successfully");
        } else {
            LOG_ROW(TAG, "Failed to start HTTPS server: %s", esp_err_to_name(ret));
        }
    } else {
        LOG_ROW(TAG, "Wi-Fi not connected, skipping network services");
    }
}

void initialize_peripheral_drivers(void) {
    // Reinitialize HX711 in RTOS mode with filters
    LOG_ROW(TAG, "Initializing HX711 in RTOS mode...");
    ESP_ERROR_CHECK(hx711_init_rtos(&g_scale, HX711_DOUT_PIN, HX711_SCK_PIN, HX711_GAIN,
                                  MA_WINDOW, USE_KF, KF_Q_INIT, KF_R_INIT, 5));

    // Initialize weight manager
    LOG_ROW(TAG, "Initializing weight manager...");
    weight_manager_init(hx711_get_queue(), &g_calib);
}

void app_main(void) {
    LOG_ROW(TAG, "=== Smart-Scale Firmware Starting ===");

    // 1. Initialize NVS and release Classic BT memory
    initialize_nvs();

    // 2. Initialize Bluetooth LE stack
    initialize_bluetooth();

    // 3. Perform calibration
    perform_calibration();

    // 4. Start network services (Wi-Fi, HTTPS)
    start_network_services();

    // 5. Initialize peripheral drivers with RTOS support
    initialize_peripheral_drivers();

    LOG_ROW(TAG, "=== System Initialization Complete ===");

    // Main monitoring loop
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000));
        
        // Periodic system status logging
        LOG_ROW(TAG, "System status - Wi-Fi: %s, BLE: %s",
                wifi_is_connected() ? "Connected" : "Disconnected",
                esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_ENABLED ? "Enabled" : "Disabled");
    }
}