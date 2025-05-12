// File: main/wifi_https_server/wifi_https_server.c

#include <string.h>
#include <stdio.h>
#include "esp_log.h"
#include "esp_err.h"
#include "esp_tls.h"
#include "esp_https_server.h"
#include "mbedtls/error.h"
#include "wifi_https_server.h"
#include "hx711.h"            // for hx711_get_latest_reading()

static const char *TAG = "wif_https_srv";

// Embedded PEM blobs (via EMBED_TXTFILES in CMakeLists.txt)
extern const uint8_t fullchain_pem_start[] asm("_binary_fullchain_pem_start");
extern const uint8_t fullchain_pem_end[]   asm("_binary_fullchain_pem_end");
extern const uint8_t privkey_pem_start[]   asm("_binary_privkey_pem_start");
extern const uint8_t privkey_pem_end[]     asm("_binary_privkey_pem_end");

/** GET /weight handler */
static esp_err_t weight_get_handler(httpd_req_t *req)
{
    float w = hx711_get_latest_reading();  // driver-level API
    char buf[64];
    int len = snprintf(buf, sizeof(buf),
                       "{ \"weight\": %.2f, \"unit\": \"g\" }\n", w);
    if (len < 0) return ESP_FAIL;

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, buf, len);
    return ESP_OK;
}

static const httpd_uri_t weight_uri = {
    .uri      = "/weight",
    .method   = HTTP_GET,
    .handler  = weight_get_handler,
    .user_ctx = NULL
};

/** Global handle, visible to main.c */


void init_https_server(void)
{
    if (https_server) {
        // already running
        return;
    }

    struct httpd_ssl_config cfg = HTTPD_SSL_CONFIG_DEFAULT();

    cfg.transport_mode = HTTPD_SSL_TRANSPORT_SECURE;
    cfg.port_secure    = 8095;
    cfg.port_insecure  = 0;
    cfg.httpd.max_uri_handlers       = 8;    // Reduce if fewer endpoints
cfg.httpd.max_open_sockets       = 3;    // Reduce concurrent connections
cfg.httpd.backlog_conn           = 2;    // Reduce connection backlog
cfg.httpd.stack_size             = 8192; // Increase if needed
cfg.httpd.lru_purge_enable       = true; // Enable LRU purging
cfg.httpd.recv_wait_timeout      = 10;   // Shorter timeout
cfg.httpd.send_wait_timeout     = 10; 

    cfg.cacert_pem     = fullchain_pem_start;
    cfg.cacert_len     = fullchain_pem_end - fullchain_pem_start;
    cfg.servercert     = fullchain_pem_start;
    cfg.servercert_len = fullchain_pem_end - fullchain_pem_start;
    cfg.prvtkey_pem    = privkey_pem_start;
    cfg.prvtkey_len    = privkey_pem_end - privkey_pem_start;

    esp_err_t err = httpd_ssl_start(&https_server, &cfg);
    if (err != ESP_OK) {
         char err_buf[128];  
          mbedtls_strerror(-err, err_buf, sizeof(err_buf));
        ESP_LOGE(TAG, "SSL start failed: 0x%x (%s)", (unsigned int) (-err), err_buf);
        ESP_LOGE(TAG, "httpd_ssl_start failed: %s", esp_err_to_name(err));
        https_server = NULL;
        return;
    }

    httpd_register_uri_handler(https_server, &weight_uri);
    ESP_LOGI(TAG, "HTTPS Server started on port %d", cfg.port_secure);
}
