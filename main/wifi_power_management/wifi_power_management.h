#ifndef WIFI_POWER_MANAGEMENT_H
#define WIFI_POWER_MANAGEMENT_H

#include <stdbool.h>   // for bool
#include <stdint.h>    // for uint16_t

// Function to manage power modes for Wi-Fi and Bluetooth
void manage_power_modes(bool is_wifi_active, bool is_ble_active);

// Function to put Wi-Fi in modem sleep
void wifi_modem_sleep(void);

// Function to put Wi-Fi in light sleep
void wifi_light_sleep(void);

// Function to switch Wi-Fi to active mode
void wifi_active_mode(void);

// Function to enable BLE advertising sleep
void ble_advertising_sleep(void);

// Function to enable BLE connection sleep
void ble_connection_sleep(void);

// Function to enable BLE idle mode
void ble_idle_mode(void);

// Function to enter deep sleep for maximum power saving
void enter_deep_sleep(void);

// Function to put both Wi-Fi and BLE into light sleep during inactivity
void light_sleep(void);

// Function to manage Bluetooth advertising intervals for efficient power usage
void set_ble_advertising_interval(uint16_t min_interval, uint16_t max_interval);

#endif // WIFI_POWER_MANAGEMENT_H
