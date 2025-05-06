#include "spiffs_manager.h"
#include "esp_spiffs.h"
#include "esp_log.h"
#include "esp_sntp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "time.h"
#include "cJSON.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TAG "SPIFFS_MANAGER"
#define JSON_FILE_PATH "/spiffs/time_log.json"
#define SNTP_SERVER "pool.ntp.org"

// ──────────────────────────────────────────────
// SNTP Initialization
// ──────────────────────────────────────────────

static void initialize_sntp(void) {
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, SNTP_SERVER);
    sntp_init();
}

static void obtain_time(void) {
    initialize_sntp();
    time_t now;
    struct tm timeinfo = {0};
    int retry = 0;
    const int retry_count = 10;

    while (timeinfo.tm_year < (2020 - 1900) && ++retry < retry_count) {
        ESP_LOGI(TAG, "Waiting for NTP sync... (%d/%d)", retry, retry_count);
        vTaskDelay(pdMS_TO_TICKS(2000));
        time(&now);
        localtime_r(&now, &timeinfo);
    }

    if (timeinfo.tm_year >= (2020 - 1900)) {
        ESP_LOGI(TAG, "Time synced: %s", asctime(&timeinfo));
    } else {
        ESP_LOGW(TAG, "Time sync failed.");
    }
}

// ──────────────────────────────────────────────
// SPIFFS Initialization
// ──────────────────────────────────────────────

void spiffs_init(void) {
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount SPIFFS (%s)", esp_err_to_name(ret));
        return;
    }

    ESP_LOGI(TAG, "SPIFFS mounted successfully");
    obtain_time();
}

// ──────────────────────────────────────────────
// Timestamp Helper
// ──────────────────────────────────────────────

bool spiffs_get_current_timestamp(char *buffer, size_t max_len) {
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    if (timeinfo.tm_year < (2020 - 1900)) return false;

    strftime(buffer, max_len, "%Y-%m-%d %H:%M:%S", &timeinfo);
    return true;
}

// ──────────────────────────────────────────────
// JSON Logging Task
// ──────────────────────────────────────────────

static void time_logging_task(void *arg) {
    char timestamp[32];

    while (1) {
        // Step 1: Load existing JSON from file
        cJSON *root = NULL;
        FILE *file = fopen(JSON_FILE_PATH, "r");
        if (file) {
            fseek(file, 0, SEEK_END);
            long size = ftell(file);
            rewind(file);

            char *buffer = malloc(size + 1);
            if (buffer) {
                fread(buffer, 1, size, file);
                buffer[size] = '\0';
                root = cJSON_Parse(buffer);
                free(buffer);
            }
            fclose(file);
        }

        if (!root) {
            root = cJSON_CreateObject();
            cJSON_AddItemToObject(root, "timestamps", cJSON_CreateArray());
        }

        cJSON *array = cJSON_GetObjectItem(root, "timestamps");
        if (!cJSON_IsArray(array)) {
            array = cJSON_CreateArray();
            cJSON_ReplaceItemInObject(root, "timestamps", array);
        }

        // Step 2: Append current timestamp
        if (spiffs_get_current_timestamp(timestamp, sizeof(timestamp))) {
            cJSON_AddItemToArray(array, cJSON_CreateString(timestamp));
            ESP_LOGI(TAG, "Logged time: %s", timestamp);

            // Step 3: Save updated JSON to file
            char *json_str = cJSON_Print(root);
            if (json_str) {
                FILE *out = fopen(JSON_FILE_PATH, "w");
                if (out) {
                    fprintf(out, "%s", json_str);
                    fclose(out);
                } else {
                    ESP_LOGE(TAG, "Failed to open %s for writing", JSON_FILE_PATH);
                }
                free(json_str);
            }
        } else {
            ESP_LOGW(TAG, "System time not yet valid");
        }

        cJSON_Delete(root);
        vTaskDelay(pdMS_TO_TICKS(5000));  // Log every 5 seconds
    }
}

void start_time_logging_task(void) {
    xTaskCreate(time_logging_task, "time_logger", 8192, NULL, 5, NULL);
}
