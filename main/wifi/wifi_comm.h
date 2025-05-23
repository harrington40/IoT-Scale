// File: wifi_comm.h
#ifndef WIFI_COMM_H
#define WIFI_COMM_H

#include <stdbool.h>
#include <stddef.h>
#include "esp_err.h"
/**
 * @brief Initialise Wi‑Fi in STA mode, scan for APs, connect to the given
 *        SSID/password, and (optionally) start the TCP‑echo server.
 *
 * @param ssid      Null‑terminated Wi‑Fi SSID
 * @param password  Null‑terminated Wi‑Fi password
 */
esp_err_t wifi_comm_start(const char *ssid, const char *password);   // ← rename

/**
 * @brief Returns true once the station has obtained an IP address.
 */
bool wifi_is_connected(void);

/**
 * @brief Send raw bytes to the connected TCP client (if any).
 *
 * @return ≥0 on success (#bytes sent), <0 on error or if no client connected.
 */
int wifi_send_data(const char *data, size_t len);

void init_https_server(void);

void wifi_comm_stop(void);

#endif /* WIFI_COMM_H */














































































































































































































































