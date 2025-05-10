// File: main/weight_manager/weight_manager.c
// ---------------------------------------------------------------------------
// Enhanced weight manager with explicit state machine and debouncing
// Features:
//   - Proper handling of weight removal/replacement
//   - Debounced state transitions
//   - SNTP-aware timestamping
//   - Configurable thresholds
// ---------------------------------------------------------------------------

#include "weight_manager.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include <stdio.h>
#include <time.h>
#include <math.h>

#include "sntp_synch.h"   // for sntp_time_available()
#include "esp_sntp.h"
#include "log_utils.h"    // for LOG_ROW()
#include "calibration.h"  // for calibration_convert()

// Configuration
#define WM_DEBOUNCE_COUNT      3    // Number of consecutive readings for state change
#define WM_WAKE_THRESHOLD_G    20.0f  // Weight threshold to detect placement (grams)
#define WM_SLEEP_THRESHOLD_G   10.0f  // Weight threshold to detect removal (grams)
#define WM_SNTP_TIMEOUT_MS     5000   // Max wait for SNTP sync
#define WM_MEASURE_INTERVAL_MS 100    // Time between measurements when active

typedef enum {
    WM_STATE_NO_WEIGHT = 0,    // Waiting for weight to be placed
    WM_STATE_DEBOUNCE_ADD,     // Detected possible weight addition
    WM_STATE_MEASURING,        // Actively measuring stable weight
    WM_STATE_DEBOUNCE_REMOVE,  // Detected possible weight removal
    WM_STATE_ERROR             // Error condition
} wm_state_t;

static QueueHandle_t  wm_queue = NULL;
static calibration_t *wm_calib = NULL;
static wm_state_t    current_state = WM_STATE_NO_WEIGHT;

/** Log timestamped weight event */
static void log_weight_event(const char *event, float weight) {
    time_t now = time(NULL);
    struct tm timeinfo;
    char timestamp[32];
    
    localtime_r(&now, &timeinfo);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &timeinfo);
    
    LOG_ROW("WeightManager", "%-12s | %s | %.2f g", event, timestamp, weight);
}

/** Handle state transitions with debouncing */
static wm_state_t handle_state_transition(wm_state_t current, float weight, int *debounce_count) {
    switch(current) {
        case WM_STATE_NO_WEIGHT:
            if (weight >= WM_WAKE_THRESHOLD_G) {
                (*debounce_count)++;
                if (*debounce_count >= WM_DEBOUNCE_COUNT) {
                    log_weight_event("WEIGHT_ADDED", weight);
                    return WM_STATE_MEASURING;
                }
                return WM_STATE_DEBOUNCE_ADD;
            }
            break;
            
        case WM_STATE_DEBOUNCE_ADD:
            if (weight >= WM_WAKE_THRESHOLD_G) {
                (*debounce_count)++;
                if (*debounce_count >= WM_DEBOUNCE_COUNT) {
                    log_weight_event("WEIGHT_ADDED", weight);
                    return WM_STATE_MEASURING;
                }
            } else {
                *debounce_count = 0;
                return WM_STATE_NO_WEIGHT;
            }
            break;
            
        case WM_STATE_MEASURING:
            if (weight <= WM_SLEEP_THRESHOLD_G) {
                (*debounce_count)++;
                if (*debounce_count >= WM_DEBOUNCE_COUNT) {
                    log_weight_event("WEIGHT_REMOVED", weight);
                    return WM_STATE_NO_WEIGHT;
                }
                return WM_STATE_DEBOUNCE_REMOVE;
            }
            break;
            
        case WM_STATE_DEBOUNCE_REMOVE:
            if (weight <= WM_SLEEP_THRESHOLD_G) {
                (*debounce_count)++;
                if (*debounce_count >= WM_DEBOUNCE_COUNT) {
                    log_weight_event("WEIGHT_REMOVED", weight);
                    return WM_STATE_NO_WEIGHT;
                }
            } else {
                *debounce_count = 0;
                return WM_STATE_MEASURING;
            }
            break;
            
        case WM_STATE_ERROR:
        default:
            // Attempt recovery after error
            if (fabs(weight) < 1000.0f) {  // Sanity check
                *debounce_count = 0;
                return WM_STATE_NO_WEIGHT;
            }
            break;
    }
    return current;
}

static void weight_manager_task(void *arg) {
    (void)arg;
    
    // Wait for SNTP sync if available
    if (!sntp_time_available(pdMS_TO_TICKS(WM_SNTP_TIMEOUT_MS))) {
        LOG_ROW("WeightManager", "SNTP not synced, using local time");
    }
    
    // State machine variables
    int debounce_count = 0;
    float raw_count = 0.0f;
    float current_weight = 0.0f;
    TickType_t last_measure_time = xTaskGetTickCount();
    
    for (;;) {
        // Get new measurement
        if (xQueueReceive(wm_queue, &raw_count, portMAX_DELAY) == pdTRUE) {
            current_weight = calibration_convert(wm_calib, (int32_t)raw_count);
            
            // Handle state transition
            wm_state_t new_state = handle_state_transition(current_state, current_weight, &debounce_count);
            
            // State-specific processing
            if (new_state != current_state) {
                current_state = new_state;
                debounce_count = 0;
            }
            
            // Log measurements when in measuring state
            if (current_state == WM_STATE_MEASURING) {
                if (xTaskGetTickCount() - last_measure_time >= pdMS_TO_TICKS(WM_MEASURE_INTERVAL_MS)) {
                    LOG_ROW("WeightManager", "Weight: %.2f g", current_weight);
                    last_measure_time = xTaskGetTickCount();
                }
            }
        }
    }
}

void weight_manager_init(QueueHandle_t hx711_queue, calibration_t *calib) {
    if (!wm_queue) {
        wm_queue = hx711_queue ? hx711_queue : xQueueCreate(8, sizeof(float));
        configASSERT(wm_queue);
        
        wm_calib = calib;
        current_state = WM_STATE_NO_WEIGHT;
        
        xTaskCreatePinnedToCore(
            weight_manager_task,
            "WeightManager",
            4096,
            NULL,
            tskIDLE_PRIORITY + 2,
            NULL,
            1  // Run on core 1
        );
        
        LOG_ROW("WeightManager", "Initialized (Wake: %.1fg, Sleep: %.1fg)", 
               WM_WAKE_THRESHOLD_G, WM_SLEEP_THRESHOLD_G);
    }
}

QueueHandle_t weight_manager_get_queue(void) {
    return wm_queue;
}

wm_state_t weight_manager_get_state(void) {
    return current_state;
}