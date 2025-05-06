// File: main/wifi/wifi_comm.c
// -----------------------------------------------------------------------------
// Robust Wi-Fi STA implementation for ESP-IDF v5.4.1 that
//   • performs an active scan for a specific SSID
//   • connects once and relies on the Wi-Fi stack for auto-reconnects
//   • runs SNTP and waits up to 10 s for time-sync
//   • starts a TCP echo server after time-sync
//   • enables modem-sleep power-save once connected
//   • launches the user's HX711 weight-manager
//   • exposes wifi_is_connected() for other components
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

// HX711 + weight manager + calibration + SNTP
#include "hx711.h"               // for hx711_get_queue()
#include "weight_manager.h"      // for weight_manager_init()
#include "calibration.h"         // for calibration_t and g_calib
#include "sntp_synch.h"          // for sntp_sync_start(), sntp_time_available()

#include "log_utils.h"           // for LOG_ROW()

extern calibration_t g_calib;    // from main.c

// ─────────────── Local defines ───────────────────────────────────────────────
#define WIFI_CONNECTED_BIT  BIT0
#define WIFI_STA_START_BIT  BIT1
#define WIFI_SCAN_DONE_BIT  BIT2

static const char *TAG = "wifi_comm";
static EventGroupHandle_t s_wifi_event_group;

static char  s_target_ssid[32] = {0};
static char  s_target_pass[64] = {0};
static bool  s_scan_successful  = false;

// ─────────────── Forward declarations ────────────────────────────────────────
static void wifi_event_handler(void *arg,
                               esp_event_base_t base,
                               int32_t id,
                               void *data);
static void wifi_scan_task(void *arg);
static void wifi_connect_task(void *arg);
static void tcp_server_task(void *arg);

/* =========================================================================== */
void wifi_comm_start(const char *ssid, const char *password)
{
    strlcpy(s_target_ssid, ssid, sizeof(s_target_ssid));
    strlcpy(s_target_pass, password, sizeof(s_target_pass));

    /* 1. NVS is required by Wi-Fi */
    ESP_ERROR_CHECK(nvs_flash_init());

    /* 2. Netif + default event loop */
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    /* 3. Register event handlers */
    ESP_ERROR_CHECK(
        esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                            wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(
        esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                            wifi_event_handler, NULL, NULL));

    /* 4. Create event group for coordination */
    s_wifi_event_group = xEventGroupCreate();

    /* 5. Configure STA mode and start driver */
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    /* 6. Wait for driver start */
    xEventGroupWaitBits(s_wifi_event_group, WIFI_STA_START_BIT,
                        pdFALSE, pdTRUE, portMAX_DELAY);

    /* 7. Launch scan & connect tasks on PRO_CPU (core 1) */
    xTaskCreatePinnedToCore(wifi_scan_task,    "wifi_scan",    4096, NULL, 4, NULL, 1);
    xTaskCreatePinnedToCore(wifi_connect_task, "wifi_connect", 4096, NULL, 4, NULL, 1);
}

/* =============================================================================
 *  Event handler
 * ==========================================================================*/
static void wifi_event_handler(void *arg, esp_event_base_t base,
                               int32_t id, void *data)
{
    if (base == WIFI_EVENT) {
        /* ───── Wi-Fi events ───────────────────────────────────────── */
        if (id == WIFI_EVENT_STA_START) {
            xEventGroupSetBits(s_wifi_event_group, WIFI_STA_START_BIT);

        } else if (id == WIFI_EVENT_SCAN_DONE) {
            xEventGroupSetBits(s_wifi_event_group, WIFI_SCAN_DONE_BIT);

        } else if (id == WIFI_EVENT_STA_CONNECTED) {
            LOG_ROW(TAG, "Connected to AP \"%s\"", s_target_ssid);
            xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);

        } else if (id == WIFI_EVENT_STA_DISCONNECTED) {
            int reason = ((wifi_event_sta_disconnected_t *)data)->reason;
            LOG_ROW(TAG, "Disconnected – reason %d, retrying…", reason);
            /* driver auto-retries */
        }

    } else if (base == IP_EVENT) {
        /* ───── IP events ───────────────────────────────────────────── */
        if (id == IP_EVENT_STA_GOT_IP) {
            esp_ip4_addr_t ip = ((ip_event_got_ip_t *)data)->ip_info.ip;
            LOG_ROW(TAG, "Got IP: " IPSTR, IP2STR(&ip));

            /* ① SNTP sync */
            sntp_sync_start();
            if (!sntp_time_available(pdMS_TO_TICKS(10000))) {
                LOG_ROW(TAG, "SNTP sync timeout – continuing anyway");
            } else {
                LOG_ROW(TAG, "SNTP time synchronised");
            }

            /* ② Enable modem sleep */
            esp_wifi_set_ps(WIFI_PS_MIN_MODEM);

            /* ③ TCP echo server */
            xTaskCreate(tcp_server_task, "tcp_srv", 4096, NULL, 4, NULL);
            LOG_ROW(TAG, "TCP echo server listening on port %d", 8080);

            /* ④ Weight manager */
            weight_manager_init(hx711_get_queue(), &g_calib);
        }
    }
}

/* =============================================================================
 *  Task 1 – Active scan
 * ==========================================================================*/
static void wifi_scan_task(void *arg)
{
    xEventGroupWaitBits(s_wifi_event_group, WIFI_STA_START_BIT,
                        pdFALSE, pdTRUE, portMAX_DELAY);

    wifi_scan_config_t scan_cfg = {0};
    scan_cfg.ssid              = (uint8_t *)s_target_ssid;
    scan_cfg.show_hidden       = true;
    scan_cfg.scan_type         = WIFI_SCAN_TYPE_ACTIVE;
    scan_cfg.scan_time.active.min = 120;
    scan_cfg.scan_time.active.max = 150;

    LOG_ROW(TAG, "Scanning for \"%s\"…", s_target_ssid);
    ESP_ERROR_CHECK(esp_wifi_scan_start(&scan_cfg, false));

    xEventGroupWaitBits(s_wifi_event_group, WIFI_SCAN_DONE_BIT,
                        pdTRUE, pdTRUE, portMAX_DELAY);

    uint16_t ap_count = 0;
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
    if (ap_count == 0) {
        LOG_ROW(TAG, "Scan finished – target not found");
        vTaskDelete(NULL);
    }

    wifi_ap_record_t ap;
    for (uint16_t i = 0; i < ap_count; ++i) {
        ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_count, &ap));
        if (strcmp((char *)ap.ssid, s_target_ssid) == 0) {
            LOG_ROW(TAG, "Found \"%s\" on channel %u, RSSI %d",
                    s_target_ssid, ap.primary, ap.rssi);
            s_scan_successful = true;
            break;
        }
    }
    vTaskDelete(NULL);
}

/* =============================================================================
 *  Task 2 – Configure STA & connect
 * ==========================================================================*/
static void wifi_connect_task(void *arg)
{
    while (!s_scan_successful) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    wifi_config_t sta_cfg = {0};
    strcpy((char *)sta_cfg.sta.ssid,     s_target_ssid);
    strcpy((char *)sta_cfg.sta.password, s_target_pass);
    sta_cfg.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    sta_cfg.sta.sae_pwe_h2e        = WPA3_SAE_PWE_BOTH;
    sta_cfg.sta.pmf_cfg.capable    = true;
    sta_cfg.sta.pmf_cfg.required   = false;

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_cfg));
    LOG_ROW(TAG, "Connecting to \"%s\"…", s_target_ssid);
    ESP_ERROR_CHECK(esp_wifi_connect());

    xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT,
                        pdFALSE, pdTRUE, portMAX_DELAY);
    vTaskDelete(NULL);
}

/* =============================================================================
 *  Task 3 – TCP echo-server (port 8080)
 * ==========================================================================*/
static void tcp_server_task(void *arg)
{
    const uint16_t port = 8080;
    int srv_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (srv_sock < 0) {
        LOG_ROW(TAG, "socket() failed: errno %d", errno);
        vTaskDelete(NULL);
    }

    struct sockaddr_in serv_addr = {
        .sin_family      = AF_INET,
        .sin_addr.s_addr = htonl(INADDR_ANY),
        .sin_port        = htons(port)
    };
    if (bind(srv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) != 0) {
        LOG_ROW(TAG, "bind() failed: errno %d", errno);
        close(srv_sock);
        vTaskDelete(NULL);
    }
    listen(srv_sock, 1);

    while (true) {
        struct sockaddr_in6 client_addr; socklen_t addr_len = sizeof(client_addr);
        int client_sock = accept(srv_sock, (struct sockaddr *)&client_addr, &addr_len);
        if (client_sock < 0) continue;
        char rx_buf[128];
        int len;
        while ((len = recv(client_sock, rx_buf, sizeof(rx_buf), 0)) > 0) {
            send(client_sock, rx_buf, len, 0);
        }
        close(client_sock);
    }
}

/* =============================================================================
 *  Public helper – connection status
 * ==========================================================================*/
bool wifi_is_connected(void)
{
    return (xEventGroupGetBits(s_wifi_event_group) & WIFI_CONNECTED_BIT) != 0;
}
