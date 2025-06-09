#include "weight_detection.h"
#include "esp_log.h"
#include "freertos/task.h"
#include <math.h>
static const char *TAG = "WeightDetect";
static weight_detection_t wd;

void weight_detection_init(void) {
    wd.current_state = WEIGHT_STATE_EMPTY;
    wd.current_weight_g = 0.0f;
    wd.stable_weight_g = 0.0f;
    wd.state_entry_tick = xTaskGetTickCount();
    wd.stable_reading_count = 0;
    wd.new_weight_event = false;
    ESP_LOGI(TAG, "Weight detection initialized");
}

static void handle_state_empty(float new_weight) {
    if (new_weight > WEIGHT_DETECTION_THRESHOLD_G) {
        wd.current_state = WEIGHT_STATE_STABILIZING;
        wd.state_entry_tick = xTaskGetTickCount();
        wd.stable_reading_count = 1;
        ESP_LOGI(TAG, "Weight detected (%.2fg), stabilizing...", new_weight);
    }
}

static void handle_state_stabilizing(float new_weight) {
    // Check if weight is stable (within threshold of previous reading)
    float change_pct = fabsf(new_weight - wd.current_weight_g) / wd.current_weight_g * 100;
    
    if (change_pct < WEIGHT_CHANGE_THRESHOLD_PCT) {
        wd.stable_reading_count++;
        if (wd.stable_reading_count >= STABLE_READINGS_REQUIRED) {
            wd.current_state = WEIGHT_STATE_STABLE;
            wd.stable_weight_g = new_weight;
            wd.new_weight_event = true;
            ESP_LOGI(TAG, "Weight stabilized: %.2fg", new_weight);
        }
    } else {
        wd.stable_reading_count = 1; // Reset counter
    }
    
    // Check for timeout
    if ((xTaskGetTickCount() - wd.state_entry_tick) > pdMS_TO_TICKS(STABILIZATION_TIMEOUT_MS)) {
        wd.current_state = WEIGHT_STATE_EMPTY;
        ESP_LOGW(TAG, "Stabilization timeout, returning to EMPTY");
    }
    
    wd.current_weight_g = new_weight;
}

static void handle_state_stable(float new_weight) {
    if (new_weight < WEIGHT_DETECTION_THRESHOLD_G) {
        wd.current_state = WEIGHT_STATE_REMOVING;
        wd.state_entry_tick = xTaskGetTickCount();
        ESP_LOGI(TAG, "Weight removal detected, confirming...");
    } else {
        // Check for significant weight change
        float change_pct = fabsf(new_weight - wd.stable_weight_g) / wd.stable_weight_g * 100;
        if (change_pct > WEIGHT_CHANGE_THRESHOLD_PCT) {
            wd.current_state = WEIGHT_STATE_STABILIZING;
            wd.state_entry_tick = xTaskGetTickCount();
            wd.stable_reading_count = 1;
            ESP_LOGI(TAG, "Significant weight change detected (%.2f%%), re-stabilizing", change_pct);
        } else {
            // Update stable weight if it's a minor change
            wd.stable_weight_g = new_weight;
        }
    }
    wd.current_weight_g = new_weight;
}

static void handle_state_removing(float new_weight) {
    if (new_weight > WEIGHT_DETECTION_THRESHOLD_G) {
        wd.current_state = WEIGHT_STATE_STABLE;
        ESP_LOGI(TAG, "False removal detection, still %.2fg", new_weight);
    } else if ((xTaskGetTickCount() - wd.state_entry_tick) > pdMS_TO_TICKS(REMOVAL_CONFIRMATION_MS)) {
        wd.current_state = WEIGHT_STATE_EMPTY;
        wd.stable_weight_g = 0.0f;
        wd.new_weight_event = true;
        ESP_LOGI(TAG, "Weight confirmed removed");
    }
    wd.current_weight_g = new_weight;
}

void weight_detection_process_new_reading(float new_weight_g) {
    // First handle the new reading
    switch (wd.current_state) {
        case WEIGHT_STATE_EMPTY:
            handle_state_empty(new_weight_g);
            break;
        case WEIGHT_STATE_STABILIZING:
            handle_state_stabilizing(new_weight_g);
            break;
        case WEIGHT_STATE_STABLE:
            handle_state_stable(new_weight_g);
            break;
        case WEIGHT_STATE_REMOVING:
            handle_state_removing(new_weight_g);
            break;
    }
    
    // Then check for timeouts
    TickType_t now = xTaskGetTickCount();
    switch (wd.current_state) {
        case WEIGHT_STATE_STABILIZING:
            if ((now - wd.state_entry_tick) > pdMS_TO_TICKS(STABILIZATION_TIMEOUT_MS)) {
                wd.current_state = WEIGHT_STATE_EMPTY;
                ESP_LOGW(TAG, "Stabilization timeout, returning to EMPTY");
            }
            break;
        case WEIGHT_STATE_REMOVING:
            if ((now - wd.state_entry_tick) > pdMS_TO_TICKS(REMOVAL_CONFIRMATION_MS)) {
                wd.current_state = WEIGHT_STATE_EMPTY;
                wd.stable_weight_g = 0.0f;
                wd.new_weight_event = true;
                ESP_LOGI(TAG, "Weight confirmed removed (timeout)");
            }
            break;
        default:
            break;
    }
}

weight_state_t weight_detection_get_current_state(void) {
    return wd.current_state;
}

float weight_detection_get_stable_weight(void) {
    return wd.stable_weight_g;
}

bool weight_detection_has_new_event(void) {
    return wd.new_weight_event;
}

void weight_detection_reset_new_event(void) {
    wd.new_weight_event = false;
}