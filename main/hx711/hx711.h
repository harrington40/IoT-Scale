#ifndef HX711_H
#define HX711_H

#include <stdint.h>
#include <stdbool.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_err.h"
#include "math.h"

typedef enum {
    HX711_GAIN_128 = 1,
    HX711_GAIN_32  = 2,
    HX711_GAIN_64  = 3
} hx711_gain_t;

// Configuration constants
#define WEIGHT_REMOVED_THRESHOLD  20.0f
#define TARE_SAMPLES             10
#define AUTO_TARE_INTERVAL_MS    (5*60*1000)
#define MEDIAN_MIN_SAMPLES       3
#define MEDIAN_MAX_SAMPLES       11
#define MEDIAN_STEP_INCREASE     2
#define MAX_READ_ATTEMPTS        100
#define DEFAULT_KF_Q             0.5f
#define DEFAULT_KF_R             10.0f
#define DEFAULT_KF_P             100.0f

typedef struct {
    gpio_num_t dout_pin;
    gpio_num_t sck_pin;
    hx711_gain_t gain;
    QueueHandle_t data_queue;
    SemaphoreHandle_t mutex;
} hx711_t;

// Initialization
esp_err_t hx711_init(hx711_t *scale,
                    gpio_num_t dout,
                    gpio_num_t sck,
                    hx711_gain_t gain);

esp_err_t hx711_init_rtos(hx711_t *scale,
                         gpio_num_t dout,
                         gpio_num_t sck,
                         hx711_gain_t gain,
                         int ma_window,
                         bool use_kf,
                         float Q_init,
                         float R_init,
                         UBaseType_t task_priority);

// Core functions
esp_err_t hx711_read_raw(hx711_t *scale, int32_t *value);
esp_err_t hx711_read_filtered(hx711_t *scale, float *value);
esp_err_t hx711_tare(hx711_t *scale, uint8_t samples);

// RTOS functions
esp_err_t hx711_read_raw_rtos(hx711_t *scale, int32_t *value);
esp_err_t hx711_read_filtered_rtos(hx711_t *scale, float *value);
esp_err_t hx711_read_median_rtos(hx711_t *scale, float *value, uint8_t *window_size);

// Utility functions
bool hx711_is_weight_removed(hx711_t *scale);
float hx711_get_latest_reading(void);
QueueHandle_t hx711_get_queue(void);
void hx711_rtos_task(void *pvParameters);
// Cleanup
void hx711_deinit(hx711_t *scale);

#endif