#pragma once

#include "esp_timer.h"
#include "esp_log.h"      // for esp_log_timestamp()
#include <inttypes.h>
#include <stdio.h>

/**
 * Print a table-row:
 *   [timestamp ms] | [component tag] | [formatted message]
 */
#define LOG_ROW(comp, fmt, ...)                                         \
    do {                                                                \
        uint32_t _t = esp_log_timestamp();                              \
        printf("%8" PRIu32 " | %-13s | " fmt "\r\n",                    \
               _t, (comp), ##__VA_ARGS__);                              \
    } while (0)
