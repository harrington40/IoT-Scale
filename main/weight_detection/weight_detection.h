#ifndef WEIGHT_DETECTION_H
#define WEIGHT_DETECTION_H

#include "hx711.h"
#include <stdbool.h>

// Configuration constants
#define WEIGHT_DETECTION_THRESHOLD_G    10.0f    // Minimum weight to consider as "present"
#define STABLE_READINGS_REQUIRED        3        // Number of stable readings to confirm state
#define STABILIZATION_TIMEOUT_MS        2000     // Max time to stabilize
#define REMOVAL_CONFIRMATION_MS         500      // Time to confirm removal
#define WEIGHT_CHANGE_THRESHOLD_PCT     7.0f     // Percentage change to detect significant weight change

typedef enum {
    WEIGHT_STATE_EMPTY,          // No weight detected
    WEIGHT_STATE_STABILIZING,    // Weight detected but stabilizing
    WEIGHT_STATE_STABLE,         // Weight is stable and valid
    WEIGHT_STATE_REMOVING        // Weight is being removed
} weight_state_t;

typedef enum {
    WEIGHT_EVENT_NEW_READING,
    WEIGHT_EVENT_STABLE_TIMEOUT,
    WEIGHT_EVENT_REMOVAL_TIMEOUT
} weight_event_t;

typedef struct {
    float current_weight_g;
    float stable_weight_g;
    weight_state_t current_state;
    TickType_t state_entry_tick;
    uint8_t stable_reading_count;
    bool new_weight_event;
} weight_detection_t;

// Public API
void weight_detection_init(void);
void weight_detection_process_new_reading(float new_weight_g);
weight_state_t weight_detection_get_current_state(void);
float weight_detection_get_stable_weight(void);
bool weight_detection_has_new_event(void);
void weight_detection_reset_new_event(void);

#endif // WEIGHT_DETECTION_H