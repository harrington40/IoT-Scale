// File: main/weight_manager/weight_manager.c
// ---------------------------------------------------------------------------
// SNTP-aware weight-manager
//   • waits for first SNTP sync
//   • wait-loops until wm_queue is non-NULL
//   • creates its own queue if caller passed NULL
//   • exposes queue via weight_manager_get_queue()
//   • prints wake/removal and weight updates via LOG_ROW
// ----------------------------------------------------------------------------

#include "weight_manager.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include <stdio.h>
#include <time.h>
#include <math.h>

#include "sntp_synch.h"   // for sntp_time_available()
#include "log_utils.h"    // for LOG_ROW()
#include "calibration.h"  // for calibration_convert()

static QueueHandle_t  wm_queue = NULL;  // HX711 counts (float)
static calibration_t *wm_calib = NULL;
 QueueHandle_t g_weight_queue = NULL;
typedef enum { WM_SLEEPING = 0, WM_ACTIVE } wm_state_t;

/**
 * Print a wake/remove event with full ISO timestamp:
 *   → Wake    | 2025-05-06 12:34:56 | 123.45 g
 *   → Removed | 2025-05-06 12:35:05 |  75.00 g
 */
static void log_ts(const char *label, float w)
{
    time_t     now = 0;
    struct tm  t;
    char       buf[32];
    time(&now);
    localtime_r(&now, &t);
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &t);
    LOG_ROW("WeightManager", "%-8s | %s | %.2f g",
            label, buf, w);
}

static void weight_manager_task(void *arg)
{
    (void)arg;

    // 1) Wait for SNTP sync

    // 2) Wait for HX711 queue
    while (!wm_queue) {
        LOG_ROW("WeightManager", "Waiting for HX711 queue...");
        vTaskDelay(pdMS_TO_TICKS(200));
    }

    LOG_ROW("WeightManager", "SNTP OK, queue OK — monitoring started");

    wm_state_t     state      = WM_SLEEPING;
    TickType_t     start_tick = 0;
    const TickType_t timeout  = pdMS_TO_TICKS(WM_ACTIVE_TIMEOUT_MS);
    float          initial_wg = 0.0f;
    float          counts_f   = 0.0f;

    for (;;) {
        if (xQueueReceive(wm_queue, &counts_f, portMAX_DELAY) != pdTRUE) {
            continue;
        }

        float g = calibration_convert(wm_calib, (int32_t)counts_f);

        switch (state) {
        case WM_SLEEPING:
            if (g >= WM_WAKE_THRESHOLD_G) {
                state      = WM_ACTIVE;
                start_tick = xTaskGetTickCount();
                initial_wg = g;
                log_ts("→ Wake", g);
            }
            break;

        case WM_ACTIVE:
            if (g <= WM_SLEEP_THRESHOLD_G) {
                state = WM_SLEEPING;
                log_ts("→ Removed", initial_wg);
            } else {
                LOG_ROW("WeightManager", "Weight: %.2f g (cnt: %.1f)", g, counts_f);
                if ((xTaskGetTickCount() - start_tick) >= timeout) {
                    state = WM_SLEEPING;
                    LOG_ROW("WeightManager", "→ Sleep (timeout)");
                }
            }
            break;
        }
    }
}

void weight_manager_init(QueueHandle_t hx711_queue, calibration_t *calib)
{
      g_weight_queue = xQueueCreate(10, sizeof(float));
    if (!wm_queue) {
        wm_queue = hx711_queue
                   ? hx711_queue
                   : xQueueCreate(8, sizeof(float));
        configASSERT(wm_queue);

        wm_calib = calib;
        xTaskCreatePinnedToCore(
            weight_manager_task,
            WM_TASK_NAME,
            WM_TASK_STACK_SIZE,
            NULL,
            WM_TASK_PRIORITY,
            NULL,
            1  // pin to core 1
        );
    }
}

QueueHandle_t weight_manager_get_queue(void)
{
    return wm_queue;
}
