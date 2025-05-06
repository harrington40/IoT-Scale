#ifndef SPIFFS_MANAGER_H
#define SPIFFS_MANAGER_H

#include <stdbool.h>
#include <stddef.h>

// Initialize SPIFFS and SNTP time
void spiffs_init(void);

// Get current time as formatted string
bool spiffs_get_current_timestamp(char *buffer, size_t max_len);

// Start background task that logs time to SPIFFS every X seconds
void start_time_logging_task(void);

#endif // SPIFFS_MANAGER_H
