// File: main/wif_https_server.h

#ifndef WIF_HTTPS_SERVER_H
#define WIF_HTTPS_SERVER_H

#include "esp_http_server.h"

/**
 * @brief   (Re)start the HTTPS server on port 443.
 *          Sets the global handle `https_server`.
 */
void init_https_server(void);

/** Global server handle, NULL if not running */
extern httpd_handle_t https_server;

#endif // WIF_HTTPS_SERVER_H
