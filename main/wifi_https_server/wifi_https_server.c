// File: main/wifi_https_server/wifi_https_server.c
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <stdio.h>
#include "wifi_comm.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_https_ota.h"
#include "esp_http_client.h"
#include "esp_tls.h"
#include "esp_https_server.h"
#include "mbedtls/error.h"
#include "esp_sntp.h"
#include "esp_sntp.h"
#include "esp_http_server.h"
#include "wifi_https_server.h"
#include "hx711.h"            // for hx711_get_latest_reading()

static const char *TAG = "wif_https_srv";
httpd_handle_t https_server = NULL;
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


esp_err_t init_https_server(void)
{
    if (https_server) {
        // already running
        return ESP_OK;
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
    cfg.httpd.send_wait_timeout      = 10; 

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
        ESP_LOGE(TAG, "SSL start failed: 0x%x (%s)", (unsigned int)(-err), err_buf);
        ESP_LOGE(TAG, "httpd_ssl_start failed: %s", esp_err_to_name(err));
        https_server = NULL;
        return err;
    }

    // Register URI handlers
    err = httpd_register_uri_handler(https_server, &weight_uri);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register /weight endpoint: %s", esp_err_to_name(err));
        httpd_ssl_stop(https_server);
        https_server = NULL;
        return err;
    }

    err = httpd_register_uri_handler(https_server, &ota_uri);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register /ota endpoint: %s", esp_err_to_name(err));
        httpd_ssl_stop(https_server);
        https_server = NULL;
        return err;
    }

    ESP_LOGI(TAG, "HTTPS Server started on port %d", cfg.port_secure);
    ESP_LOGI(TAG, "Registered /weight and /ota endpoints");
    
    return ESP_OK;
}

 esp_err_t ota_post_handler(httpd_req_t *req)
{
    httpd_resp_sendstr(req, "OTA update triggered\n");
    trigger_ota_mode();  // Will block or reboot
    return ESP_OK;
}

const httpd_uri_t ota_uri = {
    .uri      = "/ota",
    .method   = HTTP_POST,
    .handler  = ota_post_handler,
    .user_ctx = NULL
};


void trigger_ota_mode(void)
{
    ESP_LOGI(TAG, "Switching to OTA mode...");

    // Optional: stop BLE if enabled
    // esp_ble_gatts_app_unregister(your_ble_app_id);

    start_wifi_for_ota();

    esp_err_t ret = start_https_ota_update();
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "OTA update successful. Rebooting...");
        esp_restart();
    } else {
        ESP_LOGE(TAG, "OTA update failed: %s", esp_err_to_name(ret));
    }
}

esp_err_t start_https_ota_update(void)
{
    esp_http_client_config_t http_config = {
        .url = "https://your-server.com/firmware.bin",
        .cert_pem = NULL,  // For testing only
        .timeout_ms = 10000,
    };

    esp_https_ota_config_t ota_config = {
        .http_config = &http_config,
    };

    esp_err_t ret = esp_https_ota(&ota_config);
    if (ret == ESP_OK) {
        ESP_LOGI("OTA", "OTA successful, restarting...");
        esp_restart();  // You may want to return ESP_OK before restarting
    } else {
        ESP_LOGE("OTA", "OTA failed: %s", esp_err_to_name(ret));
    }
    return ret;
}


void start_wifi_for_ota(void)
{
    wifi_init_sta("your_ssid", "your_password");

    while (!wifi_is_connected()) {
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    ESP_LOGI(TAG, "Wi-Fi connected, ready for OTA");
}

void wifi_init_sta(const char *ssid, const char *password)
{
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_set_mode(WIFI_MODE_STA);

    wifi_config_t wifi_config = {
        .sta = {
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
    strncpy((char *)wifi_config.sta.password, password, sizeof(wifi_config.sta.password));

    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();
    esp_wifi_connect();
}

void wifi_https_server_stop(void)
{
    if (https_server) {
        httpd_ssl_stop(https_server);
        https_server = NULL;
        ESP_LOGI(TAG, "HTTPS Server stopped");
    }
}