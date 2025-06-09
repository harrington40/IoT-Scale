#include "hx711.h"
#include "driver/gpio.h"
#include "rom/ets_sys.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef struct {
    float *buffer;
    uint8_t size;
    uint8_t index;
    bool filled;
} median_filter_t;

typedef struct {
    float x_est;    // Estimated value
    float P;        // Estimation error
    float Q;        // Process noise
    float R;        // Measurement noise
    float K;        // Kalman gain
} kalman_filter_t;

typedef struct {
    float *buffer;
    uint16_t size;
    uint16_t index;
    float sum;
    uint16_t count;
} moving_avg_t;

// Static variables
static int32_t s_offset = 0;
static float s_last_reading = 0.0f;
static TickType_t s_last_tare_time = 0;
static QueueHandle_t s_data_queue = NULL;

// Filters
static moving_avg_t s_moving_avg = {0};
static kalman_filter_t s_kalman = {0};
static median_filter_t s_median = {0};
static uint8_t s_current_median_size = MEDIAN_MIN_SAMPLES;

// Private functions
static esp_err_t init_moving_average(uint16_t window_size);
static esp_err_t init_kalman_filter(float Q, float R);
static esp_err_t init_median_filter(void);
static float update_moving_average(float new_value);
static float update_kalman_filter(float new_value);
static float update_median_filter(float new_value);
static void adjust_median_window(bool step_detected);
static inline void swap(float *a, float *b);

esp_err_t hx711_init(hx711_t *scale, gpio_num_t dout, gpio_num_t sck, hx711_gain_t gain) {
    if (!scale || !GPIO_IS_VALID_GPIO(dout) || !GPIO_IS_VALID_OUTPUT_GPIO(sck)) {
        return ESP_ERR_INVALID_ARG;
    }

    scale->dout_pin = dout;
    scale->sck_pin = sck;
    scale->gain = gain;
    scale->data_queue = NULL;
    scale->mutex = NULL;

    gpio_reset_pin(sck);
    gpio_set_direction(sck, GPIO_MODE_OUTPUT);
    gpio_set_level(sck, 0);

    gpio_reset_pin(dout);
    gpio_set_direction(dout, GPIO_MODE_INPUT);

    s_offset = 0;
    s_last_reading = 0.0f;
    s_last_tare_time = xTaskGetTickCount();

    return ESP_OK;
}

esp_err_t hx711_init_rtos(hx711_t *scale,
                         gpio_num_t dout,
                         gpio_num_t sck,
                         hx711_gain_t gain,
                         int ma_window,
                         bool use_kf,
                         float Q,
                         float R,
                         UBaseType_t task_priority) {
    esp_err_t err = hx711_init(scale, dout, sck, gain);
    if (err != ESP_OK) return err;

    // Create mutex
    scale->mutex = xSemaphoreCreateMutex();
    if (!scale->mutex) return ESP_ERR_NO_MEM;

    // Create data queue
    s_data_queue = xQueueCreate(8, sizeof(float));
    if (!s_data_queue) {
        vSemaphoreDelete(scale->mutex);
        return ESP_ERR_NO_MEM;
    }
    scale->data_queue = s_data_queue;

    // Initialize filters
    if (ma_window > 0) {
        err = init_moving_average(ma_window);
        if (err != ESP_OK) {
            vSemaphoreDelete(scale->mutex);
            vQueueDelete(s_data_queue);
            return err;
        }
    }

    if (use_kf) {
        err = init_kalman_filter(Q, R);
        if (err != ESP_OK) {
            vSemaphoreDelete(scale->mutex);
            vQueueDelete(s_data_queue);
            if (ma_window > 0) free(s_moving_avg.buffer);
            return err;
        }
    }

    err = init_median_filter();
    if (err != ESP_OK) {
        vSemaphoreDelete(scale->mutex);
        vQueueDelete(s_data_queue);
        if (ma_window > 0) free(s_moving_avg.buffer);
        return err;
    }

    // Create task
    if (xTaskCreate(hx711_rtos_task, "hx711_task", 4096, scale, task_priority, NULL) != pdPASS) {
        hx711_deinit(scale);
        return ESP_ERR_NO_MEM;
    }

    return ESP_OK;
}

static esp_err_t init_moving_average(uint16_t window_size) {
    s_moving_avg.buffer = malloc(sizeof(float) * window_size);
    if (!s_moving_avg.buffer) return ESP_ERR_NO_MEM;

    s_moving_avg.size = window_size;
    s_moving_avg.index = 0;
    s_moving_avg.sum = 0.0f;
    s_moving_avg.count = 0;
    memset(s_moving_avg.buffer, 0, sizeof(float) * window_size);

    return ESP_OK;
}

static esp_err_t init_kalman_filter(float Q, float R) {
    s_kalman.Q = Q;
    s_kalman.R = R;
    s_kalman.P = DEFAULT_KF_P;
    s_kalman.x_est = 0.0f;
    return ESP_OK;
}

static esp_err_t init_median_filter(void) {
    s_median.buffer = malloc(sizeof(float) * MEDIAN_MAX_SAMPLES);
    if (!s_median.buffer) return ESP_ERR_NO_MEM;

    s_median.size = MEDIAN_MAX_SAMPLES;
    s_median.index = 0;
    s_median.filled = false;
    memset(s_median.buffer, 0, sizeof(float) * MEDIAN_MAX_SAMPLES);

    return ESP_OK;
}

esp_err_t hx711_read_raw(hx711_t *scale, int32_t *value) {
    if (!scale || !value) return ESP_ERR_INVALID_ARG;

    // Wait for data ready
    int attempts = 0;
    while (gpio_get_level(scale->dout_pin) && ++attempts < MAX_READ_ATTEMPTS) {
        ets_delay_us(10);
    }
    if (attempts >= MAX_READ_ATTEMPTS) return ESP_ERR_TIMEOUT;

    // Clock out 24 bits
    uint32_t data = 0;
    for (uint8_t i = 0; i < 24; i++) {
        gpio_set_level(scale->sck_pin, 1);
        ets_delay_us(1);
        data = (data << 1) | gpio_get_level(scale->dout_pin);
        gpio_set_level(scale->sck_pin, 0);
        ets_delay_us(1);
    }

    // Extra pulses for gain
    for (uint8_t i = 0; i < scale->gain; i++) {
        gpio_set_level(scale->sck_pin, 1);
        ets_delay_us(1);
        gpio_set_level(scale->sck_pin, 0);
        ets_delay_us(1);
    }

    // Sign extend and return
    *value = (data & 0x800000) ? (int32_t)(data | 0xFF000000) : (int32_t)data;
    return ESP_OK;
}

esp_err_t hx711_read_filtered(hx711_t *scale, float *value) {
    if (!scale || !value) return ESP_ERR_INVALID_ARG;

    int32_t raw;
    esp_err_t err = hx711_read_raw(scale, &raw);
    if (err != ESP_OK) return err;

    float filtered = (float)(raw - s_offset);

    // Apply moving average if enabled
    if (s_moving_avg.buffer) {
        filtered = update_moving_average(filtered);
    }

    // Apply Kalman filter if enabled
    if (s_kalman.R > 0) {
        filtered = update_kalman_filter(filtered);
    }

    *value = filtered;
    s_last_reading = filtered;
    return ESP_OK;
}

esp_err_t hx711_tare(hx711_t *scale, uint8_t samples) {
    if (!scale || samples == 0) return ESP_ERR_INVALID_ARG;

    int64_t sum = 0;
    int32_t raw;
    for (uint8_t i = 0; i < samples; i++) {
        esp_err_t err = hx711_read_raw(scale, &raw);
        if (err != ESP_OK) return err;
        sum += raw;
        ets_delay_us(10000); // 10ms delay between samples
    }

    s_offset = sum / samples;
    s_last_tare_time = xTaskGetTickCount();

    // Reset filters
    if (s_moving_avg.buffer) {
        memset(s_moving_avg.buffer, 0, sizeof(float) * s_moving_avg.size);
        s_moving_avg.sum = 0.0f;
        s_moving_avg.count = 0;
        s_moving_avg.index = 0;
    }

    if (s_kalman.R > 0) {
        s_kalman.x_est = 0.0f;
        s_kalman.P = DEFAULT_KF_P;
    }

    return ESP_OK;
}

// RTOS versions with mutex protection
esp_err_t hx711_read_raw_rtos(hx711_t *scale, int32_t *value) {
    if (!scale || !value || !scale->mutex) return ESP_ERR_INVALID_ARG;

    if (xSemaphoreTake(scale->mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    esp_err_t err = hx711_read_raw(scale, value);
    xSemaphoreGive(scale->mutex);
    return err;
}

esp_err_t hx711_read_filtered_rtos(hx711_t *scale, float *value) {
    if (!scale || !value || !scale->mutex) return ESP_ERR_INVALID_ARG;

    if (xSemaphoreTake(scale->mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    esp_err_t err = hx711_read_filtered(scale, value);
    xSemaphoreGive(scale->mutex);
    return err;
}

esp_err_t hx711_read_median_rtos(hx711_t *scale, float *value, uint8_t *window_size) {
    if (!scale || !value || !scale->mutex) return ESP_ERR_INVALID_ARG;

    if (xSemaphoreTake(scale->mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    int32_t raw;
    esp_err_t err = hx711_read_raw(scale, &raw);
    if (err != ESP_OK) {
        xSemaphoreGive(scale->mutex);
        return err;
    }

    float input = (float)(raw - s_offset);
    bool step_detected = fabsf(input - s_last_reading) > (s_last_reading * 0.1f); // 10% change
    adjust_median_window(step_detected);

    *value = update_median_filter(input);
    s_last_reading = *value;

    if (window_size) {
        *window_size = s_current_median_size;
    }

    xSemaphoreGive(scale->mutex);
    return ESP_OK;
}

// Filter implementations
static float update_moving_average(float new_value) {
    if (s_moving_avg.count < s_moving_avg.size) {
        s_moving_avg.sum += new_value;
        s_moving_avg.buffer[s_moving_avg.index++] = new_value;
        s_moving_avg.count++;
        return s_moving_avg.sum / (float)s_moving_avg.count;
    }

    s_moving_avg.sum -= s_moving_avg.buffer[s_moving_avg.index];
    s_moving_avg.sum += new_value;
    s_moving_avg.buffer[s_moving_avg.index++] = new_value;

    if (s_moving_avg.index >= s_moving_avg.size) {
        s_moving_avg.index = 0;
    }

    return s_moving_avg.sum / (float)s_moving_avg.size;
}

static float update_kalman_filter(float new_value) {
    // Prediction update
    s_kalman.P += s_kalman.Q;

    // Measurement update
    s_kalman.K = s_kalman.P / (s_kalman.P + s_kalman.R);
    s_kalman.x_est += s_kalman.K * (new_value - s_kalman.x_est);
    s_kalman.P *= (1 - s_kalman.K);

    return s_kalman.x_est;
}

static float update_median_filter(float new_value) {
    s_median.buffer[s_median.index++] = new_value;
    if (s_median.index >= s_current_median_size) {
        s_median.index = 0;
        s_median.filled = true;
    }

    if (!s_median.filled) return new_value;

    // Copy buffer for sorting
    float temp[MEDIAN_MAX_SAMPLES];
    memcpy(temp, s_median.buffer, sizeof(float) * s_current_median_size);

    // Simple bubble sort
    for (int i = 0; i < s_current_median_size - 1; i++) {
        for (int j = 0; j < s_current_median_size - i - 1; j++) {
            if (temp[j] > temp[j+1]) {
                swap(&temp[j], &temp[j+1]);
            }
        }
    }

    return temp[s_current_median_size / 2];
}

static void adjust_median_window(bool step_detected) {
    if (step_detected) {
        s_current_median_size = fmin(s_current_median_size + MEDIAN_STEP_INCREASE, MEDIAN_MAX_SAMPLES);
    } else {
        s_current_median_size = fmax(s_current_median_size - 1, MEDIAN_MIN_SAMPLES);
    }
}

static inline void swap(float *a, float *b) {
    float temp = *a;
    *a = *b;
    *b = temp;
}

// Utility functions
bool hx711_is_weight_removed(hx711_t *scale) {
    float current;
    if (hx711_read_filtered_rtos(scale, &current) != ESP_OK) return false;
    return fabsf(current) < WEIGHT_REMOVED_THRESHOLD;
}

float hx711_get_latest_reading(void) {
    return s_last_reading;
}

QueueHandle_t hx711_get_queue(void) {
    return s_data_queue;
}

void hx711_deinit(hx711_t *scale) {
    if (scale) {
        if (scale->mutex) {
            vSemaphoreDelete(scale->mutex);
            scale->mutex = NULL;
        }
        if (s_moving_avg.buffer) {
            free(s_moving_avg.buffer);
            s_moving_avg.buffer = NULL;
        }
        if (s_median.buffer) {
            free(s_median.buffer);
            s_median.buffer = NULL;
        }
        if (s_data_queue) {
            vQueueDelete(s_data_queue);
            s_data_queue = NULL;
        }
    }
}

// RTOS task
void hx711_rtos_task(void *pvParameters) {
    hx711_t *scale = (hx711_t *)pvParameters;
    float reading;
    uint8_t window_size;
    TickType_t last_tare_check = xTaskGetTickCount();

    while (1) {
        // Auto-tare check
        if (xTaskGetTickCount() - last_tare_check > pdMS_TO_TICKS(AUTO_TARE_INTERVAL_MS)) {
            if (hx711_is_weight_removed(scale)) {
                hx711_tare(scale, TARE_SAMPLES);
            }
            last_tare_check = xTaskGetTickCount();
        }

        // Read and send data
        if (hx711_read_median_rtos(scale, &reading, &window_size) == ESP_OK) {
            // Send the reading through the queue
            if (scale->data_queue) {
                xQueueSend(scale->data_queue, &reading, portMAX_DELAY);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10)); // 100Hz
    }
}