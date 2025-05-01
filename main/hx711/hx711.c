#include "hx711.h"
#include "driver/gpio.h"
#include "rom/ets_sys.h"
#include <stdlib.h>
#include <math.h>
#include "moving_average.h"
#include "kalman_filter.h"

// Static variables
static int s_ma_window = 0;
static moving_avg_t s_ma;
static float *s_ma_buf = NULL;
static bool s_use_kf = false;
static KalmanFilter s_kf;

// Step-aware boosting parameters
#define STEP_THRESHOLD   1000.0f
#define BOOSTED_Q        10.0f
#define NORMAL_Q         0.5f
#define BOOST_SAMPLES    5

static int s_boost_count = 0;
static float s_last_output = 0.0f;

// Helper function to get current moving average value
static float get_moving_average_value(moving_avg_t *ma) {
    if (ma->count == 0) return 0.0f;
    return ma->sum / (float)ma->count;
}

void hx711_init(hx711_t *scale,
                gpio_num_t dout,
                gpio_num_t sck,
                hx711_gain_t gain,
                int ma_window,
                bool use_kf,
                float Q_init,
                float R_init)
{
    // GPIO setup
    scale->dout_pin = dout;
    scale->sck_pin = sck;
    scale->gain = gain;
    scale->mutex = NULL;
    scale->data_queue = NULL;

    gpio_reset_pin(sck);
    gpio_set_direction(sck, GPIO_MODE_OUTPUT);
    gpio_reset_pin(dout);
    gpio_set_direction(dout, GPIO_MODE_INPUT);

    // Moving-average setup
    s_ma_window = ma_window;
    if (ma_window > 0) {
        s_ma_buf = malloc(sizeof(float) * ma_window);
        if (s_ma_buf) {
            moving_average_init(&s_ma, s_ma_buf, ma_window);
        } else {
            s_ma_window = 0;
        }
    }

    // Kalman filter setup
    s_use_kf = use_kf;
    if (use_kf) {
        kalman_init_adaptive(&s_kf, Q_init, R_init);
    }

    // Reset boosting state
    s_boost_count = 0;
    s_last_output = 0.0f;

    // Prime filters with one raw read
    int32_t raw0 = hx711_read_raw(scale);
    if (s_ma_window > 0) {
        moving_average_update(&s_ma, (float)raw0);
    }
    if (s_use_kf) {
        kalman_update_adaptive(&s_kf, (float)raw0);
    }
    
    // Initialize last output with proper values
    s_last_output = (s_use_kf ? s_kf.x_est : 
                   (s_ma_window > 0 ? get_moving_average_value(&s_ma) : 
                   (float)raw0));
}

void hx711_init_rtos(hx711_t *scale,
    gpio_num_t dout,
    gpio_num_t sck,
    hx711_gain_t gain,
    int ma_window,
    bool use_kf,
    float Q_init,
    float R_init,
    UBaseType_t task_priority)
{
hx711_init(scale, dout, sck, gain, ma_window, use_kf, Q_init, R_init);
scale->mutex = xSemaphoreCreateMutex();
scale->data_queue = xQueueCreate(10, sizeof(float)); // Create queue internally
xTaskCreate(hx711_rtos_task, "HX711_Task", 4096, scale, task_priority, NULL);
}

void hx711_deinit(hx711_t *scale)
{
    if (scale->mutex) {
        vSemaphoreDelete(scale->mutex);
        scale->mutex = NULL;
    }
    if (scale->data_queue) {
        vQueueDelete(scale->data_queue);
        scale->data_queue = NULL;
    }
    if (s_ma_window > 0) {
        moving_average_deinit(&s_ma);
        s_ma_window = 0;
    }
}

int32_t hx711_read_raw(hx711_t *scale)
{
    // Wait for DOUT to go low (data ready)
    while (gpio_get_level(scale->dout_pin) != 0) {
        ets_delay_us(1);
    }

    // Clock out 24 bits
    uint32_t val = 0;
    for (int i = 0; i < 24; i++) {
        gpio_set_level(scale->sck_pin, 1);
        ets_delay_us(1);
        val = (val << 1) | gpio_get_level(scale->dout_pin);
        gpio_set_level(scale->sck_pin, 0);
        ets_delay_us(1);
    }

    // Extra pulses for gain setting
    for (int i = 0; i < (int)scale->gain; i++) {
        gpio_set_level(scale->sck_pin, 1);
        ets_delay_us(1);
        gpio_set_level(scale->sck_pin, 0);
        ets_delay_us(1);
    }

    // Sign-extend 24-bit to 32-bit
    return (val & 0x800000) ? (int32_t)(val | 0xFF000000) : (int32_t)val;
}

int32_t hx711_read_raw_rtos(hx711_t *scale)
{
    int32_t raw_val = 0;
    if (xSemaphoreTake(scale->mutex, pdMS_TO_TICKS(10))) {
        raw_val = hx711_read_raw(scale);
        xSemaphoreGive(scale->mutex);
    }
    return raw_val;
}

float hx711_read_filtered(hx711_t *scale)
{
    // 1) Raw reading
    int32_t raw = hx711_read_raw(scale);
    float w = (float)raw;

    // 2) Step-aware moving average
    if (s_ma_window > 0) {
        if (s_boost_count > 0 || fabsf(w - s_last_output) > STEP_THRESHOLD) {
            moving_average_reset(&s_ma, w);
        }
        w = moving_average_update(&s_ma, w);
    }

    // 3) Step-aware Kalman
    if (s_use_kf) {
        if (s_boost_count > 0 || fabsf(w - s_last_output) > STEP_THRESHOLD) {
            s_kf.Q = BOOSTED_Q;
            s_boost_count = BOOST_SAMPLES;
        } else if (s_boost_count == 1) {
            s_kf.Q = NORMAL_Q;
        }
        if (s_boost_count > 0) {
            s_boost_count--;
        }
        w = kalman_update_adaptive(&s_kf, w);
    }

    // 4) Track for next step detection
    s_last_output = w;
    return w;
}

float hx711_read_filtered_rtos(hx711_t *scale)
{
    float filtered_value = 0.0f;
    if (xSemaphoreTake(scale->mutex, pdMS_TO_TICKS(10))) {
        filtered_value = hx711_read_filtered(scale);
        xSemaphoreGive(scale->mutex);
    }
    return filtered_value;
}

void hx711_rtos_task(void *pvParameters)
{
    hx711_t *scale = (hx711_t *)pvParameters;
    float filtered_value;
    
    while (1) {
        if (xSemaphoreTake(scale->mutex, pdMS_TO_TICKS(10))) {
            filtered_value = hx711_read_filtered(scale);
            xSemaphoreGive(scale->mutex);
            
            if (scale->data_queue) {
                xQueueSend(scale->data_queue, &filtered_value, 0);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10)); // 100Hz sampling rate
    }
}