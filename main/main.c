#include <stdio.h>
#include <inttypes.h>
#include <math.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "hx711.h"
#include "calibration.h"

// Hardware Configuration
#define DOUT_PIN               GPIO_NUM_4
#define SCK_PIN                GPIO_NUM_22
#define GAIN                   HX711_GAIN_128

// Filter Configuration
#define MA_WINDOW              8       // Moving average window size
#define USE_KF                 true    // Enable Kalman filter
#define KF_Q_INIT              0.5f    // Kalman process noise
#define KF_R_INIT              1.0f    // Kalman measurement noise

// Calibration Configuration
#define TARE_SAMPLES           16
#define CALIB_DELAY_MS         5000
#define SINGLE_KNOWN_WEIGHT_G  200.0f

// Auto-zero Configuration
#define AUTO_ZERO_THRESHOLD    5.0f
#define AUTO_ZERO_COUNT        50

// RTOS Configuration
#define HX711_TASK_PRIORITY    5       // Higher priority than processing tasks
#define HX711_TASK_STACK_SIZE  4096
#define HX711_QUEUE_SIZE       10      // Weight data queue size
#define HX711_SAMPLE_RATE_MS   10      // 100Hz sampling (10ms)

// Global variables
static QueueHandle_t hx711_queue = NULL;
static calibration_t cal;
static int32_t zero = 0;

// Configure HX711 GPIOs
static void init_hx711_pins(void) {
    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << SCK_PIN,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    io_conf.pin_bit_mask = 1ULL << DOUT_PIN;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&io_conf);
}

// RTOS Task to process weight data
void weight_processing_task(void *pvParameters) {
    float counts, grams;
    int az_count = 0;
    int64_t az_sum = 0;

    while (1) {
        if (xQueueReceive(hx711_queue, &counts, portMAX_DELAY) == pdPASS) {
            grams = calibration_convert(&cal, (int32_t)counts);

            // Auto-zero logic
            if (fabsf(grams) < AUTO_ZERO_THRESHOLD) {
                az_sum += (int32_t)counts;
                az_count++;
                if (az_count >= AUTO_ZERO_COUNT) {
                    zero = (int32_t)(az_sum / az_count);
                    cal.intercept = -cal.slope * (float)zero;
                    printf("Auto-zero → raw=%" PRId32 "\n", zero);
                    az_sum = 0;
                    az_count = 0;
                }
            } else {
                az_sum = 0;
                az_count = 0;
            }

            printf("Weight: %.2f g  (counts: %.1f)\n", grams, counts);
        }
    }
}

void app_main(void) {
    // 1. Initialize hardware
    init_hx711_pins();

    // 2. Initialize HX711 with RTOS task (now without queue parameter)
    hx711_t scale;
    hx711_init_rtos(&scale, 
                   DOUT_PIN, 
                   SCK_PIN, 
                   GAIN,
                   MA_WINDOW, 
                   USE_KF, 
                   KF_Q_INIT, 
                   KF_R_INIT,
                   HX711_TASK_PRIORITY);

    // Get the queue created by hx711_init_rtos
    hx711_queue = scale.data_queue;
    if (hx711_queue == NULL) {
        printf("Failed to get HX711 data queue!\n");
        return;
    }

    // 3. Perform initial calibration
    // Tare operation
    int32_t raw[TARE_SAMPLES];
    for (int i = 0; i < TARE_SAMPLES; i++) {
        raw[i] = hx711_read_raw_rtos(&scale);
        vTaskDelay(pdMS_TO_TICKS(20));
    }
    calibration_tare(&zero, raw, TARE_SAMPLES);
    printf("Tare complete. Zero = %" PRId32 "\n", zero);

    // Single-point calibration
    printf("Place %.2f g weight on scale...\n", SINGLE_KNOWN_WEIGHT_G);
    vTaskDelay(pdMS_TO_TICKS(CALIB_DELAY_MS));
    int32_t raw_known = hx711_read_raw_rtos(&scale);
    calibration_compute(&cal, zero, raw_known, SINGLE_KNOWN_WEIGHT_G);
    printf("Calib: slope=%.5f intercept=%.3f\n", cal.slope, cal.intercept);

    // 4. Start weight processing task
    if (xTaskCreate(weight_processing_task, "Weight_Processor",
                   configMINIMAL_STACK_SIZE + 2048, NULL, 3, NULL) != pdPASS) {
        printf("Failed to create weight processing task!\n");
    }

    // 5. Main task loop
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}