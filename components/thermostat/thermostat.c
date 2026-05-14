#include "thermostat.h"
#include "bmp280.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char* TAG = "thermostat"; 

typedef struct
{
	float actual_temp;
	float target_temp;
	SemaphoreHandle_t mutex;
	TickType_t last_logging;
} thermostat_t;

static thermostat_t g_thermostat;
static bmp280_handle_t g_bmp_handle;

void i2c_bmp280_task(void *params)
{
	float temp = 0;
    while (1) {
        esp_err_t result = bmp280_get_temperature(g_bmp_handle, &temp);
        if (result != ESP_OK) {
            ESP_LOGE(TAG, "bmp280 device read failed (%s)", esp_err_to_name(result));
        } else {
			ESP_LOGI(TAG, "sensor value read");
		}
        set_actual_temp(temp);
		vTaskDelay(pdMS_TO_TICKS(30));
    }
    bmp280_delete(g_bmp_handle);
    vTaskDelete(NULL);
}

void thermostat_init(
	i2c_master_bus_handle_t bus_handle,
	float actual_temp,
	float target_temp
) {
	bmp280_config_t dev_cfg = I2C_BMP280_CONFIG_DEFAULT;
    dev_cfg.i2c_address = BMP280_ADDRESS;
	ESP_ERROR_CHECK(bmp280_init(bus_handle, &dev_cfg, &g_bmp_handle));

	g_thermostat.actual_temp = actual_temp;
	g_thermostat.target_temp = target_temp;
	g_thermostat.last_logging = xTaskGetTickCount();
	g_thermostat.mutex = xSemaphoreCreateMutex();
	assert(g_thermostat.mutex);

	xTaskCreate(i2c_bmp280_task, "BMP280", 4096, NULL, 1, NULL);
}

float get_actual_temp()
{
	xSemaphoreTake(g_thermostat.mutex, portMAX_DELAY);
	float temp = g_thermostat.actual_temp;
	xSemaphoreGive(g_thermostat.mutex);
	return temp;
}

void set_actual_temp(float temp)
{
	xSemaphoreTake(g_thermostat.mutex, portMAX_DELAY);
	g_thermostat.actual_temp = temp;
	xSemaphoreGive(g_thermostat.mutex);
}

float get_target_temp()
{
	xSemaphoreTake(g_thermostat.mutex, portMAX_DELAY);
	float temp = g_thermostat.target_temp;
	xSemaphoreGive(g_thermostat.mutex);
	return temp;
}

void set_target_temp(float temp)
{
	xSemaphoreTake(g_thermostat.mutex, portMAX_DELAY);
	g_thermostat.target_temp = temp;
	xSemaphoreGive(g_thermostat.mutex);
}

bool increase_target_temp()
{
	bool rv = true;
	xSemaphoreTake(g_thermostat.mutex, portMAX_DELAY);
	float temp = g_thermostat.target_temp + TARGET_TEMP_CHANGE_VALUE;
	if (temp > MAX_TARGET_TEMP) {
		rv = false;
	} else {
		g_thermostat.target_temp = temp;
	}
	xSemaphoreGive(g_thermostat.mutex);
	return rv;
}

bool decrease_target_temp()
{
	bool rv = true;
	xSemaphoreTake(g_thermostat.mutex, portMAX_DELAY);
	float temp = g_thermostat.target_temp - TARGET_TEMP_CHANGE_VALUE;
	if (temp < MIN_TARGET_TEMP) {
		rv = false;
	} else {
		g_thermostat.target_temp = temp;
	}
	xSemaphoreGive(g_thermostat.mutex);
	return rv;
}
