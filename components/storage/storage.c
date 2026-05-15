#include "storage.h"
#include "wifi_credentials.h"

#include "nvs.h"
#include "nvs_flash.h"

#include "esp_log.h"
#include "esp_err.h"

#include <string.h>

static const char *TAG = "storage";

#define NAMESPACE "config"

#define KEY_TARGET_TEMP "target_temp"

#define KEY_WIFI_SSID "wifi_ssid"
#define KEY_WIFI_PASS "wifi_pass"

static nvs_handle_t g_nvs_handle;

void storage_init(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(nvs_open(NAMESPACE, NVS_READWRITE, &g_nvs_handle));

    ESP_LOGI(TAG, "NVS initialized");
    if (FORCE_NEW_WIFI || !storage_has_wifi_credentials()) {
        ESP_LOGW(TAG, "Forcing new WiFi credentials");
        ESP_ERROR_CHECK(nvs_set_str(g_nvs_handle, KEY_WIFI_SSID, WIFI_SSID));
        ESP_ERROR_CHECK(nvs_set_str(g_nvs_handle, KEY_WIFI_PASS, WIFI_PASSWORD));
        ESP_ERROR_CHECK(nvs_commit(g_nvs_handle));
    }
}

bool storage_has_wifi_credentials(void)
{
    char ssid[32];
    size_t size = sizeof(ssid);

    esp_err_t err = nvs_get_str(g_nvs_handle, KEY_WIFI_SSID, ssid, &size);
    return err == ESP_OK;
}

wifi_credentials_t storage_get_wifi_credentials(void)
{
	wifi_credentials_t creds = {0};
	size_t ssid_max_size = sizeof(creds.ssid);
	size_t pass_max_size = sizeof(creds.password);

	ESP_ERROR_CHECK(
		nvs_get_str(g_nvs_handle, KEY_WIFI_SSID, creds.ssid, &ssid_max_size)
	);
	ESP_ERROR_CHECK(
		nvs_get_str(g_nvs_handle, KEY_WIFI_PASS, creds.password, &pass_max_size)
	);

	return creds;
}

float storage_get_target_temp(void)
{
    float temp = 25.0f;
    size_t size = sizeof(temp);
    esp_err_t err = nvs_get_blob(g_nvs_handle, KEY_TARGET_TEMP, &temp, &size);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Target temp missing, using default");
		temp = 25.0f;
    }
    return temp;
}

void storage_set_target_temp(float temp)
{
    ESP_ERROR_CHECK(
        nvs_set_blob(g_nvs_handle, KEY_TARGET_TEMP, &temp, sizeof(temp))
	);
    ESP_ERROR_CHECK(nvs_commit(g_nvs_handle));

    ESP_LOGI(TAG, "Saved target temp: %.2f", temp);
}
