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

static const char *TAG = "web_server";

void web_init(void)
{
    // start_webserver() is called inside wifi event handler
    wifi_init();
}

static const char* HTML_PAGE =
"<!doctype html>"
"<html>"
"<head>"
"<title>Thermostat</title>"
"<script>"
"async function refresh(){"
"  const r = await fetch('/api/state');"
"  const d = await r.json();"
"  document.getElementById('t').innerText = d.target.toFixed(2);"
"  document.getElementById('a').innerText = d.actual.toFixed(2);"
"  document.getElementById('h').innerText = d.heater;"
"  const rt = await fetch('/api/time');"
"  const dt = await rt.json();"
"  document.getElementById('time').innerText = dt.time;"
"}"
"async function up(){ await fetch('/api/target/up', {method:'POST'}); refresh(); }"
"async function down(){ await fetch('/api/target/down', {method:'POST'}); refresh(); }"
"setInterval(refresh, 1000);"
"</script>"
"</head>"
"<body onload='refresh()'>"
"<h1>Thermostat</h1>"
"<p>Time: <span id='time'></span></p>"
"<p>Target: <span id='t'></span> C</p>"
"<p>Actual: <span id='a'></span> C</p>"
"<p>Heater: <span id='h'></span> %</p>"
"<button onclick='up()'>+</button>"
"<button onclick='down()'>-</button>"
"</body>"
"</html>";

static esp_err_t root_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, HTML_PAGE, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t state_get_handler(httpd_req_t *req)
{
    char json[128];

    snprintf(json, sizeof(json),
        "{\"target\":%.2f,\"actual\":%.2f,\"heater\":%d}",
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

static esp_err_t time_get_handler(httpd_req_t *req)
{
    struct tm timeinfo;
    char buffer[64];
    get_time(&timeinfo);
    time_to_str(&timeinfo, buffer);

    char json[128];
    snprintf(json, sizeof(json),
        "{\"time\":\"%s\"}", buffer);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json, HTTPD_RESP_USE_STRLEN);

    return ESP_OK;
}

static esp_err_t logs_handler(httpd_req_t *req)
{
    FILE *f = fopen("/spiffs/log.csv", "r");
    if (!f) {
        httpd_resp_send_404(req);
        return ESP_OK;
    }

    httpd_resp_set_type(req, "text/csv");

    char line[128];
    while (fgets(line, sizeof(line), f)) {
        httpd_resp_sendstr_chunk(req, line);
    }

    httpd_resp_sendstr_chunk(req, NULL);
    fclose(f);

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

    static const httpd_uri_t time_uri = {
        .uri = "/api/time",
        .method = HTTP_GET,
        .handler = time_get_handler,
    };
    httpd_register_uri_handler(server, &time_uri);

    return server;
}
