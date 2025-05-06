// File: main/main.c
// ---------------------------------------------------------------------------
// Smart-Scale startup:
//   1) Single-point tare + calibration  
//   2) HX711 RTOS driver → queue  
//   3) Weight manager → timestamped logging (immediate)  
//   4) Wi-Fi → SNTP → echo server → power-save  
// ---------------------------------------------------------------------------

#include <stdio.h>
#include <inttypes.h>            // for PRId32
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

// Utility for nice tabular logs
#include "log_utils.h"           // defines LOG_ROW(comp, fmt, ...)

// Wi-Fi
#include "wifi_comm.h"

// HX711 driver
#include "hx711.h"
#include "driver/gpio.h"

// Weight manager
#include "weight_manager.h"

// Calibration
#include "calibration.h"

/* ---------- App-wide definitions ---------------------------------------- */
#define WIFI_SSID           "Tori_2.44Ghz"
#define WIFI_PASS           "Logarithmses900"

// HX711 pins/settings
#define HX711_DOUT_PIN      GPIO_NUM_4
#define HX711_SCK_PIN       GPIO_NUM_22
#define HX711_GAIN          HX711_GAIN_128

// Filters
#define MA_WINDOW           8
#define USE_KF              true
#define KF_Q_INIT           0.5f
#define KF_R_INIT           1.0f

// Calibration samples & weight
#define CAL_SAMPLES         10
#define CAL_KNOWN_WEIGHT_G  200.0f

static const char *TAG = "app_main";

// Global HX711 and calibration objects (external linkage)
hx711_t       g_scale;
calibration_t g_calib;    // filled during app_main calibration step

void app_main(void)
{
    LOG_ROW(TAG, "=== Smart-Scale FW starting ===");

    // 1) Basic tare + single-point calibration
    {
        int32_t raw_buf[CAL_SAMPLES];
        int32_t zero_raw      = 0;
        int32_t raw_at_weight = 0;

        // Temporary init (no RTOS task) to get raw readings
        hx711_init(&g_scale,
                   HX711_DOUT_PIN,
                   HX711_SCK_PIN,
                   HX711_GAIN,
                   MA_WINDOW,
                   USE_KF,
                   KF_Q_INIT,
                   KF_R_INIT);

        for (int i = 0; i < CAL_SAMPLES; i++) {
            raw_buf[i] = hx711_read_raw(&g_scale);
            vTaskDelay(pdMS_TO_TICKS(100));
        }

        calibration_tare(&zero_raw, raw_buf, CAL_SAMPLES);
        LOG_ROW(TAG, "Tare complete, zero_raw = %" PRId32, zero_raw);

        LOG_ROW(TAG, "Place %.0f g known weight on scale…", CAL_KNOWN_WEIGHT_G);
        vTaskDelay(pdMS_TO_TICKS(5000));  // give user time

        raw_at_weight = hx711_read_raw(&g_scale);
        LOG_ROW(TAG, "Raw at weight = %" PRId32, raw_at_weight);

        calibration_compute(&g_calib,
                            zero_raw,
                            raw_at_weight,
                            CAL_KNOWN_WEIGHT_G);
        LOG_ROW(TAG, "Calibration: slope=%.6f intercept=%.2f",
                g_calib.slope, g_calib.intercept);
    }

    // 2) Start HX711 RTOS driver (creates hx711 queue internally)
    hx711_init_rtos(&g_scale,
                    HX711_DOUT_PIN,
                    HX711_SCK_PIN,
                    HX711_GAIN,
                    MA_WINDOW,
                    USE_KF,
                    KF_Q_INIT,
                    KF_R_INIT,
                    5);  // task priority

    // 3) Start the weight manager immediately so you see live readings
    weight_manager_init(hx711_get_queue(), &g_calib);

    // 4) Finally, bring up Wi-Fi, SNTP, echo server, power-save, etc.
    wifi_comm_start(WIFI_SSID, WIFI_PASS);

    // Idle forever
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
