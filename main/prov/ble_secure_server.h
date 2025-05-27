#ifndef BLE_SECURE_SERVER_H
#define BLE_SECURE_SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GATTS_SERVICE_UUID        0x00FF
#define GATTS_CHAR_UUID_A         0xFF01
#define GATTS_CHAR_UUID_B         0xFF02
#define GATTS_CHAR_UUID_C         0xFF03
#define GATTS_NUM_HANDLE          8

#define DEVICE_NAME               "ESP32_SECURE_SERVER"

/* Attributes State Machine */
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

void ble_secure_server_init(void);

#endif // BLE_SECURE_SERVER_H
