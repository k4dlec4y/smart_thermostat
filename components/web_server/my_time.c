#include "my_time.h"
#include "esp_log.h"

static const char *TAG = "my_time";

void init_sntp(void)
{
    ESP_LOGI(TAG, "Initializing SNTP");

	setenv("TZ", "CET-1CEST,M3.5.0/2,M10.5.0/3", 1);
    tzset();

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

void get_time(struct tm *timeinfo)
{
	assert(timeinfo != NULL);

    time_t now;
    time(&now);
    localtime_r(&now, timeinfo);
}

void time_to_str(struct tm *timeinfo, char buffer[64])
{
	assert(timeinfo != NULL);
	assert(buffer != NULL);

    strftime(buffer, 64, "%Y-%m-%d %H:%M:%S", timeinfo);
}
