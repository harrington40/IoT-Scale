// File: main/wif_https_server.h

#ifndef WIF_HTTPS_SERVER_H
#define WIF_HTTPS_SERVER_H

#include "esp_http_server.h"
 #include "esp_err.h"
#include "esp_http_server.h"

/**
 * @brief   (Re)start the HTTPS server on port 443.
 *          Sets the global handle `https_server`.
 */


// Declare the OTA POST handler
esp_err_t ota_post_handler(httpd_req_t *req);

// Declare the URI handler structure for HTTP server
extern const httpd_uri_t ota_uri;

// Triggers OTA update mode
void trigger_ota_mode(void);

// Starts HTTPS OTA update process
esp_err_t start_https_ota_update(void);

// Starts Wi-Fi in STA mode for OTA
void start_wifi_for_ota(void);

// Initializes Wi-Fi with given SSID and password
void wifi_init_sta(const char *ssid, const char *password);

// Stops the running HTTPS server instance
void wifi_https_server_stop(void);

// Extern declaration of HTTPS server handle (if declared globally in .c)
extern struct httpd_ssl_config_t https_server_config;
void init_https_server(void);

/** Global server handle, NULL if not running */
extern httpd_handle_t https_server;

#endif // WIF_HTTPS_SERVER_H
