// File: main/wifi/wifi_comm.c
// -----------------------------------------------------------------------------
// Robust Wi-Fi STA implementation for ESP-IDF v5.4.1 with HTTPS JSON endpoint
// -----------------------------------------------------------------------------

#include "wifi_comm.h"
#include <string.h>
#include <errno.h>

#include "esp_err.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "lwip/sockets.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

// HTTPS server
#include "esp_http_server.h"
#include "esp_https_server.h"
#include "server_cert.h"      // Embedded PEM blobs

// HX711 + weight manager + calibration + SNTP
#include "hx711.h"            // for hx711_get_queue()
#include "weight_manager.h"   // for weight_manager_init()
#include "calibration.h"      // for calibration_t and g_calib
#include "sntp_synch.h"       // for sntp_sync_start(), sntp_time_available()
#include "log_utils.h"        // for LOG_ROW()

extern calibration_t g_calib;    // from main.c

#define WIFI_CONNECTED_BIT   BIT0
#define WIFI_STA_START_BIT   BIT1
#define WIFI_SCAN_DONE_BIT   BIT2

static const char *TAG = "wifi_comm";
static EventGroupHandle_t s_wifi_event_group;
static EventGroupHandle_t https_server_events;
static TaskHandle_t        https_server_task_handle;

static char s_target_ssid[32];
static char s_target_pass[64];
static bool s_scan_successful = false;
static httpd_handle_t https_server = NULL;

// HTTPS server task config
#define HTTPS_SERVER_TASK_STACK_SIZE 8192
#define HTTPS_SERVER_TASK_PRIORITY   5
#define SERVER_RESTART_DELAY_MS      5000

// Server control bits
#define SERVER_START_BIT  (1 << 0)
#define SERVER_ERROR_BIT  (1 << 2)

#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

// PEM chain marker for verifying CA chain
static const char *chain_marker = "-----END CERTIFICATE-----\n-----BEGIN CERTIFICATE-----";

// Forward declarations
static void     wifi_event_handler(void *arg, esp_event_base_t base, int32_t id, void *data);
static void     wifi_scan_task(void *arg);
static void     wifi_connect_task(void *arg);
static void     https_server_monitor(void *arg);
static void     start_https_server_task(void *pvParameters);
static esp_err_t weight_get_handler(httpd_req_t *req);
static void     start_https_server(void);
static void     stop_https_server(void);

esp_err_t wifi_comm_start(const char *ssid, const char *password)
{
    strlcpy(s_target_ssid, ssid, sizeof(s_target_ssid));
    strlcpy(s_target_pass, password, sizeof(s_target_pass));

    // NVS for Wi-Fi
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    // Wi-Fi driver init
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Register event handlers
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL, NULL));

    // Create event groups
    s_wifi_event_group    = xEventGroupCreate();
    https_server_events   = xEventGroupCreate();

    // Start Wi-Fi
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    // Wait until STA start done
    xEventGroupWaitBits(s_wifi_event_group,
                        WIFI_STA_START_BIT,
                        pdFALSE, pdTRUE,
                        portMAX_DELAY);

    // Launch scan, connect, and HTTPS server tasks
    xTaskCreatePinnedToCore(wifi_scan_task, "wifi_scan", 4096, NULL, 4, NULL, 1);
    xTaskCreatePinnedToCore(wifi_connect_task, "wifi_conn", 4096, NULL, 4, NULL, 1);
    xTaskCreate(https_server_monitor,    "https_monitor", 4096, NULL, HTTPS_SERVER_TASK_PRIORITY-1, NULL);
    xTaskCreate(start_https_server_task, "https_server",  HTTPS_SERVER_TASK_STACK_SIZE, NULL, HTTPS_SERVER_TASK_PRIORITY, &https_server_task_handle);

    return ESP_OK;
}

static void wifi_event_handler(void *arg, esp_event_base_t base,
                               int32_t id, void *data)
{
    if (base == WIFI_EVENT) {
        switch (id) {
            case WIFI_EVENT_STA_START:
                xEventGroupSetBits(s_wifi_event_group, WIFI_STA_START_BIT);
                break;
            case WIFI_EVENT_SCAN_DONE:
                xEventGroupSetBits(s_wifi_event_group, WIFI_SCAN_DONE_BIT);
                break;
            case WIFI_EVENT_STA_CONNECTED:
                LOG_ROW(TAG, "Connected to '%s'", s_target_ssid);
                xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
                break;
            case WIFI_EVENT_STA_DISCONNECTED:
                LOG_ROW(TAG, "Disconnected, reconnecting...");
                xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
                esp_wifi_connect();
                break;
            default:
                break;
        }
    }
    else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) data;
        LOG_ROW(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));

        // SNTP sync
        sntp_sync_start();
        if (sntp_time_available(pdMS_TO_TICKS(10000))) {
            LOG_ROW(TAG, "SNTP time synchronised");
        } else {
            LOG_ROW(TAG, "SNTP sync timeout, continuing");
        }

        // Modem-sleep power save
        esp_wifi_set_ps(WIFI_PS_MIN_MODEM);

        // Start HTTPS server and weight manager
        start_https_server();
        weight_manager_init(hx711_get_queue(), &g_calib);
    }
}

static void wifi_scan_task(void *arg)
{
    xEventGroupWaitBits(s_wifi_event_group, WIFI_STA_START_BIT,
                        pdFALSE, pdTRUE, portMAX_DELAY);

    wifi_scan_config_t scan_cfg = {
        .ssid        = (uint8_t *)s_target_ssid,
        .show_hidden = true,
        .scan_type   = WIFI_SCAN_TYPE_ACTIVE,
        .scan_time.active = {
            .min = 120,
            .max = 150
        }
    };

    LOG_ROW(TAG, "Scanning for '%s'...", s_target_ssid);
    ESP_ERROR_CHECK(esp_wifi_scan_start(&scan_cfg, false));
    xEventGroupWaitBits(s_wifi_event_group, WIFI_SCAN_DONE_BIT,
                        pdTRUE, pdTRUE, portMAX_DELAY);

    uint16_t ap_count = 0;
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
    if (ap_count == 0) {
        LOG_ROW(TAG, "Target SSID not found");
        vTaskDelete(NULL);
    }

    wifi_ap_record_t ap;
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_count, &ap));
    if (strcmp((char *)ap.ssid, s_target_ssid) == 0) {
        LOG_ROW(TAG, "Found '%s', RSSI %d", s_target_ssid, ap.rssi);
        s_scan_successful = true;
    }
    vTaskDelete(NULL);
}

static void wifi_connect_task(void *arg)
{
    while (!s_scan_successful) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    wifi_config_t sta_cfg = { .sta = {
        .threshold.authmode = WIFI_AUTH_WPA2_PSK
    }};
    strlcpy((char *)sta_cfg.sta.ssid,     s_target_ssid, sizeof(sta_cfg.sta.ssid));
    strlcpy((char *)sta_cfg.sta.password, s_target_pass, sizeof(sta_cfg.sta.password));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_cfg));

    LOG_ROW(TAG, "Connecting to '%s'...", s_target_ssid);
    ESP_ERROR_CHECK(esp_wifi_connect());

    xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT,
                        pdFALSE, pdTRUE, portMAX_DELAY);
    vTaskDelete(NULL);
}

static esp_err_t weight_get_handler(httpd_req_t *req)
{
    float weight;
    if (xQueueReceive(hx711_get_queue(), &weight, 0) == pdPASS) {
        char json[64];
        int len = snprintf(json, sizeof(json), "{\"weight\":%.2f}", weight);
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, json, len);
    } else {
        httpd_resp_send_500(req);
    }
    return ESP_OK;
}

static void https_server_monitor(void *arg)
{
    while (1) {
        xEventGroupWaitBits(https_server_events,
                            SERVER_START_BIT | SERVER_ERROR_BIT,
                            pdTRUE, pdFALSE,
                            portMAX_DELAY);

        if (xEventGroupGetBits(https_server_events) & SERVER_ERROR_BIT) {
            LOG_ROW(TAG, "Server error, restarting in %d ms", SERVER_RESTART_DELAY_MS);
            vTaskDelay(pdMS_TO_TICKS(SERVER_RESTART_DELAY_MS));
            xEventGroupSetBits(https_server_events, SERVER_START_BIT);
        }
    }
}

static void start_https_server(void)
{
    xEventGroupSetBits(https_server_events, SERVER_START_BIT);
}

static void stop_https_server(void)
{
    if (https_server) {
        ESP_LOGI(TAG, "Stopping HTTPS server");
        httpd_ssl_stop(https_server);
        https_server = NULL;
    }
}

static void start_https_server_task(void *pvParameters)
{
    while (1) {
        xEventGroupWaitBits(https_server_events,
                            SERVER_START_BIT,
                            pdTRUE, pdFALSE,
                            portMAX_DELAY);

        // 1) Validate embedded PEM blobs
        if (server_cert_chain_len == 0 || server_private_key_len == 0) {
            ESP_LOGE(TAG, "Certificate or key is zero-length");
            xEventGroupSetBits(https_server_events, SERVER_ERROR_BIT);
            continue;
        }

        // 2) Verify PEM headers
        if (!memmem(server_cert_chain_pem, server_cert_chain_len,
                    "-----BEGIN CERTIFICATE-----", strlen("-----BEGIN CERTIFICATE-----")) ||
            !memmem(server_private_key_pem, server_private_key_len,
                    "-----BEGIN PRIVATE KEY-----", strlen("-----BEGIN PRIVATE KEY-----"))) {
            ESP_LOGE(TAG, "Missing PEM header");
            xEventGroupSetBits(https_server_events, SERVER_ERROR_BIT);
            continue;
        }

        // 3) Verify CA chain marker
        if (!memmem(server_cert_chain_pem, server_cert_chain_len,
                    chain_marker, strlen(chain_marker))) {
            ESP_LOGE(TAG, "Missing CA chain marker");
            xEventGroupSetBits(https_server_events, SERVER_ERROR_BIT);
            continue;
        }

        // 4) Configure and start HTTPS server
        httpd_ssl_config_t conf = HTTPD_SSL_CONFIG_DEFAULT();
        conf.cacert_pem  = (const unsigned char*)server_cert_chain_pem;
        conf.cacert_len  = server_cert_chain_len;
        conf.prvtkey_pem = (const unsigned char*)server_private_key_pem;
        conf.prvtkey_len = server_private_key_len;
        conf.httpd.server_port      = 8095;
        conf.httpd.stack_size       = 10240;
        conf.httpd.lru_purge_enable = true;
        conf.httpd.max_open_sockets = 6;
        conf.httpd.ctrl_port        = 32768;
        conf.transport_mode         = HTTPD_SSL_TRANSPORT_SECURE;
        conf.port_insecure          = 0;
        conf.session_tickets        = false;

        esp_err_t ret = httpd_ssl_start(&https_server, &conf);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Server start failed (0x%x): %s", ret, esp_err_to_name(ret));
            xEventGroupSetBits(https_server_events, SERVER_ERROR_BIT);
            continue;
        }

        // 5) Register URI handler
        httpd_uri_t uri = {
            .uri      = "/weight",
            .method   = HTTP_GET,
            .handler  = weight_get_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(https_server, &uri);

        ESP_LOGI(TAG, "✅ HTTPS server started on port %d", conf.httpd.server_port);

        // 6) Monitor server
        while (https_server) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
        xEventGroupSetBits(https_server_events, SERVER_ERROR_BIT);
    }
}

bool wifi_is_connected(void)
{
    return (xEventGroupGetBits(s_wifi_event_group) & WIFI_CONNECTED_BIT) != 0;
}

void wifi_comm_stop(void)
{
    stop_https_server();
    esp_wifi_disconnect();
    esp_wifi_stop();
    esp_wifi_deinit();

    if (s_wifi_event_group) {
        vEventGroupDelete(s_wifi_event_group);
        s_wifi_event_group = NULL;
    }
    if (https_server_events) {
        vEventGroupDelete(https_server_events);
        https_server_events = NULL;
    }
    if (https_server_task_handle) {
        vTaskDelete(https_server_task_handle);
        https_server_task_handle = NULL;
    }
}

void init_https_server(void) {
    // Create event group for server control if it doesn't exist
    if (https_server_events == NULL) {
        https_server_events = xEventGroupCreate();
        if (https_server_events == NULL) {
            ESP_LOGE(TAG, "Failed to create HTTPS server event group");
            return;
        }
    }

    // Create server monitor task if it doesn't exist
    if (https_server_task_handle == NULL) {
        BaseType_t ret = xTaskCreate(
            start_https_server_task,
            "https_server",
            HTTPS_SERVER_TASK_STACK_SIZE,
            NULL,
            HTTPS_SERVER_TASK_PRIORITY,
            &https_server_task_handle
        );

        if (ret != pdPASS) {
            ESP_LOGE(TAG, "Failed to create HTTPS server task");
            vEventGroupDelete(https_server_events);
            https_server_events = NULL;
            return;
        }
    }

    // Create server monitor task
    TaskHandle_t monitor_handle;
    if (xTaskCreate(
            https_server_monitor,
            "https_monitor",
            8192,
            NULL,
            HTTPS_SERVER_TASK_PRIORITY-1,
            &monitor_handle) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create HTTPS monitor task");
    }

    ESP_LOGI(TAG, "HTTPS server components initialized");
}