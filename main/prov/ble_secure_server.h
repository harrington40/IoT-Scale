#ifndef BLE_SECURE_SERVER_H
#define BLE_SECURE_SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_gatts_api.h"
#include "esp_gap_ble_api.h"
#include "esp_bt_defs.h"

/* UUIDs and Attribute Count */

#define GATTS_CHAR_UUID_A         0xFF01
#define GATTS_CHAR_UUID_B         0xFF02
#define GATTS_CHAR_UUID_C         0xFF03
#define GATTS_NUM_HANDLE          8

#define DEVICE_NAME               "SecureGATTDevice"  // Matches .c file name string

/* Attributes State Machine Indexes */
enum
{
    IDX_SVC,

    IDX_CHAR_A,
    IDX_CHAR_VAL_A,
    IDX_CHAR_CFG_A,

    IDX_CHAR_B,
    IDX_CHAR_VAL_B,

    IDX_CHAR_C,
    IDX_CHAR_VAL_C,

    HRS_IDX_NB,
};

/* Function to initialize BLE Secure Server */
#ifdef __cplusplus
extern "C" {
#endif

void ble_secure_server_init(void);

#ifdef __cplusplus
}
#endif

#endif // BLE_SECURE_SERVER_H
