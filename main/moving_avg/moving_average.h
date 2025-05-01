#ifndef MOVING_AVERAGE_H
#define MOVING_AVERAGE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

typedef struct {
    float* buffer;        // circular buffer storage
    int    size;          // window length
    int    index;         // next write position
    float  sum;           // running sum for average
    int    count;         // number of valid entries (≤ size)

    // for median3 prefilter
    float  last_vals[3];  // last three inputs
    int    last_count;    // how many of last_vals are filled (≤3)
    
    // RTOS additions
    SemaphoreHandle_t mutex; // Thread safety
} moving_avg_t;

// Original functions
float median3(float a, float b, float c);
void moving_average_init(moving_avg_t* filter, float* buffer, int size);
float moving_average_update(moving_avg_t* filter, float new_value);
float moving_average_update_median3(moving_avg_t* filter, float new_value);
void moving_average_reset(moving_avg_t* filter, float init_value);
int moving_average_get_window_size(moving_avg_t* filter);
void moving_average_deinit(moving_avg_t* filter);

// RTOS-specific functions
void moving_average_init_rtos(moving_avg_t* filter, float* buffer, int size);
float moving_average_update_rtos(moving_avg_t* filter, float new_value);
float moving_average_update_median3_rtos(moving_avg_t* filter, float new_value);
void moving_average_reset_rtos(moving_avg_t* filter, float init_value);

#ifdef __cplusplus
}
#endif

#endif // MOVING_AVERAGE_H