// File: main/wifi/wifi_comm.c
// -----------------------------------------------------------------------------
// Robust Wi-Fi STA implementation for ESP-IDF v5.4.x
// -----------------------------------------------------------------------------

#include "wifi_comm.h"
#include <string.h>

#include "esp_err.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "lwip/inet.h"  

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

static const char *TAG = "wifi_comm";
static EventGroupHandle_t s_wifi_event_group;

/** EventGroup bit to indicate that we have an IP */
#define WIFI_CONNECTED_BIT BIT0

// Forward declaration for our event handler
static void wifi_event_handler(void* arg,
                               esp_event_base_t event_base,
                               int32_t event_id,
                               void* event_data);

esp_err_t wifi_comm_start(const char *ssid, const char *password)
{
    // 1) Init NVS (required by Wi-Fi)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 2) Init TCP/IP and default event loop
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // 3) Create default Wi-Fi STA netif
    esp_netif_create_default_wifi_sta();

    // 4) Init Wi-Fi with default config
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // 5) Create an EventGroup to track connection
    s_wifi_event_group = xEventGroupCreate();

    // 6) Register our event handler for Wi-Fi and IP events
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP,    &wifi_event_handler, NULL, NULL));

    // 7) Configure and start Wi-Fi in STA mode
    wifi_config_t wifi_cfg = {
        .sta = {
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    strncpy((char*)wifi_cfg.sta.ssid,     ssid,     sizeof(wifi_cfg.sta.ssid) - 1);
    strncpy((char*)wifi_cfg.sta.password, password, sizeof(wifi_cfg.sta.password) - 1);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Wi-Fi initialized, connecting to \"%s\" …", ssid);
    // Connection attempt is automatic via event handler on STA_START

    return ESP_OK;
}

bool wifi_is_connected(void)
{
    return (xEventGroupGetBits(s_wifi_event_group) & WIFI_CONNECTED_BIT) != 0;
}

void wifi_comm_stop(void)
{
    ESP_LOGI(TAG, "Stopping Wi-Fi");
    esp_wifi_stop();
    esp_wifi_deinit();
    if (s_wifi_event_group) {
        vEventGroupDelete(s_wifi_event_group);
        s_wifi_event_group = NULL;
    }
}

//-----------------------------------------------------------------------------
// Internal event handler: responds to Wi-Fi and IP events
//-----------------------------------------------------------------------------
static void wifi_event_handler(void* arg,
                               esp_event_base_t event_base,
                               int32_t event_id,
                               void* event_data)
{
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_START:
                esp_wifi_connect();
                ESP_LOGI(TAG, "WIFI_EVENT_STA_START: connecting …");
                break;
            case WIFI_EVENT_STA_DISCONNECTED:
                ESP_LOGW(TAG, "WIFI_EVENT_STA_DISCONNECTED: disconnected, retrying …");
                esp_wifi_connect();
                xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
                break;
            default:
                break;
        }
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* info = (ip_event_got_ip_t*) event_data;
        
        
      ESP_LOGI(TAG,
                 "IP_EVENT_STA_GOT_IP: " IPSTR,
                 IP2STR(&info->ip_info.ip));
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}
