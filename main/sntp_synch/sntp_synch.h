// sntp_sync.h
#pragma once
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

void  sntp_sync_start(void);
bool   sntp_time_available(TickType_t timeout);
void  initialize_sntp(void);
bool  is_sntp_time_set(TickType_t timeout);