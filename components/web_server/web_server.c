#include "web_server.h"
#include "storage.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "thermostat.h"
#include <string.h>

static const char *TAG = "web_server";

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
}

static const char* HTML_TEMPLATE = 
    "<html>"
    "<head><title>ESP32 Thermostat</title></head>"
    "<body>"
    "<h1>Thermostat Monitor</h1>"
    "<p>Target Temp: <b>%.2f C</b></p>"
    "<p>Actual Temp: <b>%.2f C</b></p>"
    "<p>Heater Output: <b>%d%%</b></p>"
    "</body>"
    "</html>";

static esp_err_t root_get_handler(httpd_req_t *req)
{
    float target = get_target_temp();
	float actual = get_actual_temp();
	int heater_pwm = get_heater_pwm();

    char response[512];
    snprintf(response, sizeof(response), HTML_TEMPLATE, target, actual, heater_pwm);

    httpd_resp_send(req, response, HTTPD_RESP_USE_STRLEN);
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

    return server;
}
