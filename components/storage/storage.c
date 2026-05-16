#include "storage.h"
#include "wifi_credentials.h"

#include "nvs.h"
#include "nvs_flash.h"
#include "esp_vfs_fat.h"
#include "wear_levelling.h"
#include "unistd.h"
#include "sys/stat.h"
#include "my_time.h"
#include "thermostat.h"

#include "esp_log.h"
#include "esp_err.h"

#include <string.h>

static const char *TAG = "STORAGE";

/* NVS */
static const char *NAMESPACE = "config";
static const char *KEY_TARGET_TEMP = "target_temp";
static const char *KEY_WIFI_SSID = "wifi_ssid";
static const char *KEY_WIFI_PASS = "wifi_pass";

const char *LOG_FILE = "/fatfs/log.csv";
const char *OLD_LOG_FILE = "/fatfs/log_old.csv";

#define FILE_MAX_SIZE 256 * 1024

static nvs_handle_t g_nvs_handle;
static wl_handle_t s_wl_handle = WL_INVALID_HANDLE;

static void ensure_log_file_exists(const char *path)
{
    if (access(path, F_OK) == 0)
        return;

    ESP_LOGI(TAG, "Creating %s", path);
    FILE *file = fopen(path, "w");
    fclose(file);
}

void storage_init(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND
    ) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(nvs_open(NAMESPACE, NVS_READWRITE, &g_nvs_handle));

    ESP_LOGI(TAG, "NVS initialized");
    if (!storage_has_wifi_credentials() || FORCE_NEW_WIFI) {
        ESP_LOGW(TAG, "Forcing new WiFi credentials");
        ESP_ERROR_CHECK(nvs_set_str(g_nvs_handle, KEY_WIFI_SSID, WIFI_SSID));
        ESP_ERROR_CHECK(nvs_set_str(g_nvs_handle, KEY_WIFI_PASS, WIFI_PASSWORD));
        ESP_ERROR_CHECK(nvs_commit(g_nvs_handle));
    }

    const esp_vfs_fat_mount_config_t mount_config = {
        .format_if_mount_failed = true,
        .max_files = 4,
        .allocation_unit_size = CONFIG_WL_SECTOR_SIZE,
    };

    esp_err_t err = esp_vfs_fat_spiflash_mount_rw_wl(
        "/fatfs",
        "storage",
        &mount_config,
        &s_wl_handle
    );

    ESP_ERROR_CHECK(err);

    ESP_LOGI(TAG, "FatFs mounted");
    ensure_log_file_exists(LOG_FILE);
    ensure_log_file_exists(OLD_LOG_FILE);
}

static void log_rotate()
{
    struct stat st;
    if (stat(LOG_FILE, &st) != 0) {
        ESP_LOGE(TAG, "Could not get %s file's size using stat()", LOG_FILE);
        return;
    }

    if (st.st_size < FILE_MAX_SIZE)
        return;

    if (remove(OLD_LOG_FILE) != 0) {
        ESP_LOGE(TAG, "Could not remove %s", OLD_LOG_FILE);
        return; 
    }
    if (rename(LOG_FILE, OLD_LOG_FILE) != 0) {
        ESP_LOGE(TAG, "Could not move %s to %s", LOG_FILE, OLD_LOG_FILE);
        return;
    }
    if (truncate(LOG_FILE, 0) != 0)
        ESP_LOGE(TAG, "Could not truncate %s", LOG_FILE);
    return;
}

static void logger_task(void *arg)
{
    while (atomic_load(&is_time_set_up))
    {}
    ESP_LOGI(TAG, "Logger task start");
    while (1) {

        time_t now;
        time(&now);

        struct tm tm_info;
        localtime_r(&now, &tm_info);

        int wait_sec = (15 - (tm_info.tm_min % 15)) * 60 - tm_info.tm_sec;
        if (wait_sec <= 0)
            wait_sec += 15 * 60;

        vTaskDelay(pdMS_TO_TICKS(wait_sec * 1000));

        get_time(&tm_info);
        char timestamp[64];
        time_to_str(&tm_info, timestamp);

        FILE *file = fopen(LOG_FILE, "a");
        if (!file)
            continue;

        fprintf(file, "%s,%.2f,%.2f,%d\n",
            timestamp,
            get_actual_temp(),
            get_target_temp(),
            (int)get_heater_pwm()
        );
        fclose(file);

        ESP_LOGI(TAG, "Log written");
        log_rotate();
    }

    vTaskDelete(NULL);
}

void start_logger_task(void)
{
    xTaskCreate(logger_task, "Logger", 4096, NULL, 1, NULL);
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

    ESP_LOGI(TAG, "Target temp has been saved: %.2f", temp);
}
