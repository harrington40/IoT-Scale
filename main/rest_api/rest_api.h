// File: rest_api.h
#ifndef REST_API_H
#define REST_API_H

#include "esp_err.h"

/**
 * @brief  Starts the RESTful HTTP server. Call after Wi‑Fi is connected.
 *
 * @return ESP_OK on success, esp_err_t on failure.
 */
esp_err_t rest_server_start(void);

/**
 * @brief  Stops the RESTful HTTP server and frees resources.
 */
void     rest_server_stop(void);

#endif // REST_API_H