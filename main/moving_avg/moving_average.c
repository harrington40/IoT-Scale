#include "moving_average.h"
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

/* Helper function: median of three values */
float median3(float a, float b, float c) {
    if (a > b) { float t = a; a = b; b = t; }
    if (b > c) { float t = b; b = c; c = t; }
    if (a > b) { float t = a; a = b; b = t; }
    return b;
}

/* Basic initialization (non-RTOS) */
void moving_average_init(moving_avg_t* filter, float* buffer, int size) {
    if (!filter || !buffer || size <= 0) return;
    
    filter->buffer = buffer;
    filter->size = size;
    filter->index = 0;
    filter->sum = 0.0f;
    filter->count = 0;
    filter->last_count = 0;
    filter->mutex = NULL; // No mutex for basic version
    
    memset(filter->buffer, 0, size * sizeof(float));
}

/* RTOS initialization (creates mutex) */
void moving_average_init_rtos(moving_avg_t* filter, float* buffer, int size) {
    moving_average_init(filter, buffer, size);
    filter->mutex = xSemaphoreCreateMutex();
}

/* Core moving average update (non-thread-safe) */
float moving_average_update(moving_avg_t* filter, float new_value) {
    if (!filter || !filter->buffer || filter->size <= 0) return 0.0f;
    
    // Subtract outgoing value
    filter->sum -= filter->buffer[filter->index];
    
    // Add incoming value
    filter->buffer[filter->index] = new_value;
    filter->sum += new_value;
    
    // Update index and count
    filter->index = (filter->index + 1) % filter->size;
    if (filter->count < filter->size) {
        filter->count++;
    }
    
    return filter->sum / (float)filter->count;
}

/* Thread-safe moving average update */
float moving_average_update_rtos(moving_avg_t* filter, float new_value) {
    if (!filter) return 0.0f;
    
    float result = 0.0f;
    
    if (filter->mutex && xSemaphoreTake(filter->mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        result = moving_average_update(filter, new_value);
        xSemaphoreGive(filter->mutex);
    } else if (!filter->mutex) {
        result = moving_average_update(filter, new_value);
    }
    
    return result;
}

/* Median prefilter + moving average (non-thread-safe) */
float moving_average_update_median3(moving_avg_t* filter, float new_value) {
    if (!filter) return 0.0f;
    
    // Update median buffer
    if (filter->last_count < 3) {
        filter->last_vals[filter->last_count++] = new_value;
    } else {
        filter->last_vals[0] = filter->last_vals[1];
        filter->last_vals[1] = filter->last_vals[2];
        filter->last_vals[2] = new_value;
    }
    
    // Compute median or pass-through
    float med = (filter->last_count == 3) ? 
        median3(filter->last_vals[0], filter->last_vals[1], filter->last_vals[2]) : 
        new_value;
    
    return moving_average_update(filter, med);
}

/* Thread-safe median prefilter + moving average */
float moving_average_update_median3_rtos(moving_avg_t* filter, float new_value) {
    if (!filter) return 0.0f;
    
    float result = 0.0f;
    
    if (filter->mutex && xSemaphoreTake(filter->mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        result = moving_average_update_median3(filter, new_value);
        xSemaphoreGive(filter->mutex);
    } else if (!filter->mutex) {
        result = moving_average_update_median3(filter, new_value);
    }
    
    return result;
}

/* Reset filter (non-thread-safe) */
void moving_average_reset(moving_avg_t* filter, float init_value) {
    if (!filter || !filter->buffer || filter->size <= 0) return;
    
    memset(filter->buffer, 0, filter->size * sizeof(float));
    filter->buffer[0] = init_value;
    filter->sum = init_value;
    filter->count = 1;
    filter->index = 1 % filter->size;
    
    // Prime median history
    filter->last_vals[0] = init_value;
    filter->last_vals[1] = init_value;
    filter->last_vals[2] = init_value;
    filter->last_count = 3;
}

/* Thread-safe reset */
void moving_average_reset_rtos(moving_avg_t* filter, float init_value) {
    if (!filter) return;
    
    if (filter->mutex && xSemaphoreTake(filter->mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        moving_average_reset(filter, init_value);
        xSemaphoreGive(filter->mutex);
    } else if (!filter->mutex) {
        moving_average_reset(filter, init_value);
    }
}

/* Get window size (thread-safe by nature) */
int moving_average_get_window_size(moving_avg_t* filter) {
    return filter ? filter->size : 0;
}

/* Cleanup resources */
void moving_average_deinit(moving_avg_t* filter) {
    if (!filter) return;
    
    // Free RTOS resources if they exist
    if (filter->mutex) {
        vSemaphoreDelete(filter->mutex);
        filter->mutex = NULL;
    }
    
    // Free buffer if it exists
    if (filter->buffer) {
        free(filter->buffer);
        filter->buffer = NULL;
    }
    
    // Reset all fields
    filter->size = 0;
    filter->index = 0;
    filter->sum = 0.0f;
    filter->count = 0;
    filter->last_count = 0;
}