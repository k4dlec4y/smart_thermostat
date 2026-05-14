#include "thermostat.h"
#include "bmp280.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/ledc.h"

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

float pid_update(PID *pid, float set_temp, float act_temp)
{
    float dt = 1;
    float error = set_temp - act_temp;
    float P = pid->Kp * error;
    pid->integral += error * dt;
    if (pid->integral > 100) pid->integral = 100;
    if (pid->integral < -100) pid->integral = -100;
    float I = pid->Ki * pid->integral;
    float derivative = (error - pid->prev_error) / dt;
    float D = pid->Kd * derivative;
    pid->prev_error = error;
    float output = P + I + D;
    if (output > 100)
        output = 100;
    if (output < 0)
        output = 0;
    return output;
}

void heater_pwm_init(void)
{
	ledc_timer_config_t timer = {
		.speed_mode = LEDC_LOW_SPEED_MODE,
		.timer_num = LEDC_TIMER_0,
		.duty_resolution = LEDC_TIMER_10_BIT,
		.freq_hz = 5000,
		.clk_cfg = LEDC_AUTO_CLK
	};

	ESP_ERROR_CHECK(ledc_timer_config(&timer));

	ledc_channel_config_t channel = {
		.gpio_num = HEATER_PWM_PIN,
		.speed_mode = LEDC_LOW_SPEED_MODE,
		.channel = LEDC_CHANNEL_0,
		.timer_sel = LEDC_TIMER_0,
		.duty = 0,
		.hpoint = 0
	};

	ESP_ERROR_CHECK(ledc_channel_config(&channel));
}

void heater_pwm_set(int duty_percentage)
{
	ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty_percentage * 1023 / 100);
	ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}

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
