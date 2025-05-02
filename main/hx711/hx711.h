#ifndef HX711_H
#define HX711_H

#include <stdint.h>
#include <stdbool.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

typedef enum {
    HX711_GAIN_128 = 1,
    HX711_GAIN_32  = 2,
    HX711_GAIN_64  = 3
} hx711_gain_t;

typedef struct {
    gpio_num_t dout_pin;
    gpio_num_t sck_pin;
    hx711_gain_t gain;
    QueueHandle_t data_queue;   
    SemaphoreHandle_t mutex;  
} hx711_t;

// Basic initialization
void hx711_init(hx711_t *scale,
                gpio_num_t dout,
                gpio_num_t sck,
                hx711_gain_t gain,
                int ma_window,
                bool use_kf,
                float Q_init,
                float R_init);

// RTOS initialization (without queue parameter)
void hx711_init_rtos( hx711_t     *scale,
    gpio_num_t   dout,
    gpio_num_t   sck,
    hx711_gain_t gain,
    int          ma_window,
    bool         use_kf,
    float        Q_init,
    float        R_init,
    UBaseType_t  task_priority );
// Alternative RTOS initialization (with queue parameter)
void hx711_init_rtos_with_queue(hx711_t *scale,
                               gpio_num_t dout,
                               gpio_num_t sck,
                               hx711_gain_t gain,
                               int ma_window,
                               bool use_kf,
                               float Q_init,
                               float R_init,
                               UBaseType_t task_priority,
                               QueueHandle_t data_queue);

// Median filter declaration (implementation in .c)
// Add to existing HX711 RTOS functions
int32_t hx711_read_median_rtos(hx711_t *scale, uint8_t* current_sample_size);
void hx711_rtos_median_task(void *pvParameters);  // New RTOS task for median filtering


// Add these near other #defines
#define MEDIAN_MIN_SAMPLES  3    // Minimum median window size
#define MEDIAN_MAX_SAMPLES  11   // Maximum during rapid changes
#define MEDIAN_STEP_INCREASE 2   // Samples to add when step detected

// Read functions
int32_t hx711_read_raw(hx711_t *scale);
float hx711_read_filtered(hx711_t *scale);
void hx711_rtos_task(void *pvParameters);

// RTOS read functions
int32_t hx711_read_raw_rtos(hx711_t *scale);
float hx711_read_filtered_rtos(hx711_t *scale);

// Cleanup
void hx711_deinit(hx711_t *scale);

#endif // HX711_H