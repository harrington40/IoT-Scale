// File: rest_api.c
#include "rest_api.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "cJSON.h"
#include "weight_manager.h"
#include "esp_timer.h"
#include "esp_system.h"

static const char *TAG = "REST_API";
static httpd_handle_t server_handle = NULL;

// GET /weight → { "weight_g": <float> }
static esp_err_t get_weight_handler(httpd_req_t *req)
{
    float weight = weight_manager_get_weight();
    cJSON *root = cJSON_CreateObject();
    if (!root) {
        ESP_LOGE(TAG, "JSON create failed");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    cJSON_AddNumberToObject(root, "weight_g", weight);
    char *json_str = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (!json_str) {
        ESP_LOGE(TAG, "JSON print failed");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_str, strlen(json_str));
    free(json_str);
    return ESP_OK;
}

// POST /config → accept { "threshold": <float> }, respond 204
static esp_err_t post_config_handler(httpd_req_t *req)
{
    int len = req->content_len;
    char *buf = malloc(len + 1);
    if (!buf) {
        ESP_LOGE(TAG, "malloc failed");
        httpd_resp_send_500(req);
        return ESP_ERR_NO_MEM;
    }
    int ret = httpd_req_recv(req, buf, len);
    if (ret <= 0) {
        free(buf);
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    buf[ret] = 0;
    cJSON *json = cJSON_Parse(buf);
    free(buf);
    if (!json) {
        ESP_LOGW(TAG, "Invalid JSON");
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }
    cJSON *jthr = cJSON_GetObjectItem(json, "threshold");
    if (cJSON_IsNumber(jthr)) {
        weight_manager_set_threshold(jthr->valuedouble);
    } else {
        ESP_LOGW(TAG, "Missing threshold field");
        cJSON_Delete(json);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing 'threshold'");
        return ESP_FAIL;
    }
    cJSON_Delete(json);
    httpd_resp_set_status(req, "204 No Content");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

// GET /config → { "threshold": <float> }
static esp_err_t get_config_handler(httpd_req_t *req)
{
    float threshold = weight_manager_get_threshold();
    cJSON *root = cJSON_CreateObject();
    if (!root) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    cJSON_AddNumberToObject(root, "threshold", threshold);
    char *json_str = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (!json_str) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_str, strlen(json_str));
    free(json_str);
    return ESP_OK;
}

// GET /health → { "uptime_s": <float>, "free_heap": <uint32> }
static esp_err_t get_health_handler(httpd_req_t *req)
{
    uint64_t uptime_us = esp_timer_get_time();
    uint32_t free_heap = esp_get_free_heap_size();
    cJSON *root = cJSON_CreateObject();
    if (!root) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    cJSON_AddNumberToObject(root, "uptime_s", uptime_us / 1e6);
    cJSON_AddNumberToObject(root, "free_heap", free_heap);
    char *json_str = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (!json_str) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_str, strlen(json_str));
    free(json_str);
    return ESP_OK;
}

// URI registration
static const httpd_uri_t weight_uri = {
    .uri      = "/weight",
    .method   = HTTP_GET,
    .handler  = get_weight_handler,
    .user_ctx = NULL
};
static const httpd_uri_t config_post_uri = {
    .uri      = "/config",
    .method   = HTTP_POST,
    .handler  = post_config_handler,
    .user_ctx = NULL
};
static const httpd_uri_t config_get_uri = {
    .uri      = "/config",
    .method   = HTTP_GET,
    .handler  = get_config_handler,
    .user_ctx = NULL
};
static const httpd_uri_t health_uri = {
    .uri      = "/health",
    .method   = HTTP_GET,
    .handler  = get_health_handler,
    .user_ctx = NULL
};

esp_err_t rest_server_start(void)
{
    if (server_handle) {
        ESP_LOGW(TAG, "Server already running");
        return ESP_ERR_INVALID_STATE;
    }
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port      = 80;
    config.max_uri_handlers = 8;
    config.ctrl_port        = 0;

    esp_err_t ret = httpd_start(&server_handle, &config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "httpd_start failed: %s", esp_err_to_name(ret));
        return ret;
    }
    httpd_register_uri_handler(server_handle, &weight_uri);
    httpd_register_uri_handler(server_handle, &config_post_uri);
    httpd_register_uri_handler(server_handle, &config_get_uri);
    httpd_register_uri_handler(server_handle, &health_uri);

    ESP_LOGI(TAG, "REST server started on port %d", config.server_port);
    return ESP_OK;
}

void rest_server_stop(void)
{
    if (server_handle) {
        httpd_stop(server_handle);
        server_handle = NULL;
        ESP_LOGI(TAG, "REST server stopped");
    }
}
