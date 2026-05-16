#include "web_server.h"
#include "storage.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "thermostat.h"
#include "my_time.h"
#include "wifi.h"

#include <string.h>

static const char *TAG = "WEB_SERVER";

void web_init(void)
{
    // start_webserver() is called inside wifi event handler
    wifi_init();
}

static esp_err_t root_get_handler(httpd_req_t *req)
{
    extern const uint8_t index_html_start[] asm("_binary_index_html_start");
    extern const uint8_t index_html_end[]   asm("_binary_index_html_end");
    httpd_resp_set_type(req, "text/html");
    size_t len = index_html_end - index_html_start;
    httpd_resp_send(req, (const char*)index_html_start, len);
    return ESP_OK;
}

static esp_err_t app_get_handler(httpd_req_t *req)
{
    extern const uint8_t app_html_start[] asm("_binary_app_js_start");
    extern const uint8_t app_html_end[]   asm("_binary_app_js_end");
    httpd_resp_set_type(req, "application/javascript");
    size_t len = app_html_end - app_html_start;
    httpd_resp_send(req, (const char*)app_html_start, len);
    return ESP_OK;
}

static esp_err_t favicon_get_handler(httpd_req_t *req)
{
    extern const uint8_t favicon_html_start[] asm("_binary_favicon_ico_start");
    extern const uint8_t favicon_html_end[]   asm("_binary_favicon_ico_end");
    httpd_resp_set_type(req, "image/x-icon");
    size_t len = favicon_html_end - favicon_html_start;
    httpd_resp_send(req, (const char*)favicon_html_start, len);
    return ESP_OK;
}

static esp_err_t state_get_handler(httpd_req_t *req)
{
    struct tm timeinfo;
    char buffer[64];
    get_time(&timeinfo);
    time_to_str(&timeinfo, buffer);

    char json[256];
    snprintf(json, sizeof(json),
        "{\"time\":\"%s\",\"target\":%.2f,\"actual\":%.2f,\"heater\":%d}",
        buffer,
        get_target_temp(),
        get_actual_temp(),
        (int)get_heater_pwm());

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json, HTTPD_RESP_USE_STRLEN);

    return ESP_OK;
}

static esp_err_t target_up_handler(httpd_req_t *req)
{
    increase_target_temp();
    storage_set_target_temp(get_target_temp());

    httpd_resp_send(req, "OK", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t target_down_handler(httpd_req_t *req)
{
    decrease_target_temp();
    storage_set_target_temp(get_target_temp());

    httpd_resp_send(req, "OK", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t logs_get_handler(httpd_req_t *req)
{
    #define FILES_LEN 2
    const char *files[FILES_LEN] = {
        OLD_LOG_FILE,
        LOG_FILE
    };

    for (size_t i = 0; i < FILES_LEN; ++i) {
        FILE *file = fopen(files[i], "r");
        if (!file) {
            httpd_resp_send_404(req);
            return ESP_OK;
        }
        httpd_resp_set_type(req, "text/csv");

        char buf[256];
        size_t n;
        while ((n = fread(buf, 1, sizeof(buf), file)) > 0) {
            httpd_resp_send_chunk(req, buf, n);
        }

        fclose(file);
    }

    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;

    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) != ESP_OK) {
        ESP_LOGE(TAG, "Error starting server!");
        return NULL;
    }

    const httpd_uri_t root_uri = {
        .uri       = "/",
        .method    = HTTP_GET,
        .handler   = root_get_handler,
        .user_ctx  = NULL
    };
    httpd_register_uri_handler(server, &root_uri);

    const httpd_uri_t app_uri = {
        .uri       = "/app.js",
        .method    = HTTP_GET,
        .handler   = app_get_handler,
        .user_ctx  = NULL
    };
    httpd_register_uri_handler(server, &app_uri);

    const httpd_uri_t favicon_uri = {
        .uri       = "/favicon.ico",
        .method    = HTTP_GET,
        .handler   = favicon_get_handler,
        .user_ctx  = NULL
    };
    httpd_register_uri_handler(server, &favicon_uri);

    const httpd_uri_t state_uri = {
        .uri = "/api/state",
        .method = HTTP_GET,
        .handler = state_get_handler,
    };
    httpd_register_uri_handler(server, &state_uri);

    const httpd_uri_t up_uri = {
        .uri = "/api/target/up",
        .method = HTTP_POST,
        .handler = target_up_handler,
    };
    httpd_register_uri_handler(server, &up_uri);

    static const httpd_uri_t down_uri = {
        .uri = "/api/target/down",
        .method = HTTP_POST,
        .handler = target_down_handler,
    };
    httpd_register_uri_handler(server, &down_uri);

    static const httpd_uri_t logs_uri = {
        .uri = "/logs.csv",
        .method = HTTP_GET,
        .handler = logs_get_handler,
    };
    httpd_register_uri_handler(server, &logs_uri);

    return server;
}
