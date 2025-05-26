// weight_manager.h

#ifndef WEIGHT_MANAGER_H_
#define WEIGHT_MANAGER_H_

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "calibration.h"

#ifdef __cplusplus
extern "C" {
#endif


#ifndef QueueHandle_t
typedef struct QueueDefinition *QueueHandle_t;
#endif

extern QueueHandle_t g_weight_queue;

// Weight Manager thresholds (grams)
#ifndef WM_WAKE_THRESHOLD_G
#define WM_WAKE_THRESHOLD_G     200.0f
#endif

#ifndef WM_SLEEP_THRESHOLD_G
#define WM_SLEEP_THRESHOLD_G    10.0f
#endif

// Active timeout in milliseconds
#ifndef WM_ACTIVE_TIMEOUT_MS
#define WM_ACTIVE_TIMEOUT_MS    60000
#endif

// FreeRTOS task configuration
#ifndef WM_TASK_NAME
#define WM_TASK_NAME            "WeightManager"
#endif

#ifndef WM_TASK_STACK_SIZE
#define WM_TASK_STACK_SIZE      2048
#endif

#ifndef WM_TASK_PRIORITY
#define WM_TASK_PRIORITY        3
#endif

/**
 * @brief Initialize the weight manager.
 * @param hx711_queue  QueueHandle_t from hx711_init_rtos()
 * @param calib        Pointer to calibration_t struct for conversion
 *
 * Spawns a FreeRTOS task that:
 *  - Sleeps (no output) until weight ≥ WM_WAKE_THRESHOLD_G
 *  - On wake, prints weights and runs for WM_ACTIVE_TIMEOUT_MS
 *  - Returns to sleep on weight removal (< WM_SLEEP_THRESHOLD_G) or timeout
 */
void weight_manager_init(QueueHandle_t hx711_queue, calibration_t *calib);

#ifdef __cplusplus
}
#endif

#endif // WEIGHT_MANAGER_H_
