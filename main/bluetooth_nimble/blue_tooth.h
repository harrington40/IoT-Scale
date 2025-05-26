#ifndef BLUETOOTH_NIMBLE_H
#define BLUETOOTH_NIMBLE_H

#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t flags;
    uint16_t weight;
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
} ble_weight_measurement_t;

esp_err_t bluetooth_nimble_init(void);
void bluetooth_update_weight(float weight, bool stable);
bool bluetooth_is_connected(void);
esp_err_t bluetooth_nimble_deinit(void);

#ifdef __cplusplus
}
#endif

#endif // BLUETOOTH_NIMBLE_H