#include "web_server.h"
#include "storage.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "thermostat.h"
#include "esp_sntp.h"
#include "time.h"
#include <string.h>

static const char *TAG = "web_server";

static void init_sntp(void)
{
    ESP_LOGI(TAG, "Initializing SNTP");

    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_init();

    time_t now = 0;
    struct tm timeinfo = { 0 };

    int retry = 0;
    const int retry_count = 10;

    while (timeinfo.tm_year < (2020 - 1900) && ++retry < retry_count) {
        ESP_LOGI(TAG, "Waiting for system time...");
        vTaskDelay(pdMS_TO_TICKS(1000));

        time(&now);
        localtime_r(&now, &timeinfo);
    }

    ESP_LOGI(TAG, "Time synchronized");
}

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                          int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        esp_wifi_connect();
        ESP_LOGI(TAG, "Retrying connection to AP...");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));

        // Start web server here once we have an IP
        init_sntp();
        start_webserver();
    }
}

void wifi_and_web_init(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
		WIFI_EVENT,
		ESP_EVENT_ANY_ID,
		&wifi_event_handler,
		NULL,
		&instance_any_id
	));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
		IP_EVENT,
		IP_EVENT_STA_GOT_IP,
		&wifi_event_handler,
		NULL,
		&instance_got_ip
	));

	wifi_credentials_t creds = storage_get_wifi_credentials();
	ESP_LOGI(TAG, "wifi ssid: %s", creds.ssid);
	ESP_LOGI(TAG, "password ssid: %s", creds.password);
    wifi_config_t wifi_config;
    memset(&wifi_config, 0, sizeof(wifi_config_t));

    strncpy((char*)wifi_config.sta.ssid, creds.ssid, sizeof(wifi_config.sta.ssid));
    strncpy((char*)wifi_config.sta.password, creds.password, sizeof(wifi_config.sta.password));

    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK; 
	
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    setenv("TZ", "CET-1CEST,M3.5.0/2,M10.5.0/3", 1);
    tzset();
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
    time_t now;
    time(&now);

    struct tm timeinfo;
    localtime_r(&now, &timeinfo);

    char buf[64];
    strftime(buf, sizeof(buf),
             "%Y-%m-%d %H:%M:%S",
             &timeinfo);

    char json[128];
    snprintf(json, sizeof(json),
        "{\"time\":\"%s\"}", buf);

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
