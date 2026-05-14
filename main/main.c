#include "driver/i2c_master.h"
#include "esp_log.h"
#include "display.h"
#include "thermostat.h"
#include "buttons.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "main";

#define I2C_SDA 17
#define I2C_SCL 18

static i2c_master_bus_handle_t i2c_bus_handle = NULL;

void i2c_init(void)
{
	i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = I2C_SDA,
        .scl_io_num = I2C_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    ESP_ERROR_CHECK(
        i2c_new_master_bus(&bus_config, &i2c_bus_handle)
    );
}

void app_main(void)
{
    float default_actual_temp = 15;
    float default_target_temp = 25;

	i2c_init();
    thermostat_init(i2c_bus_handle, default_actual_temp, default_target_temp);
    display_init(i2c_bus_handle, default_actual_temp, default_target_temp);
    buttons_init();
    heater_pwm_init();

    PID pid = { .Kp = 2.0f, .Ki = 0.5f, .Kd = 1.0f };

    while (1) {
        float act_temp = get_actual_temp();
        display_set_actual_temp(act_temp);
        float set_temp = get_target_temp();
        display_set_target_temp(set_temp);

        if (atomic_load(&button_down_pressed)) {
            atomic_store(&button_down_pressed, false);
            decrease_target_temp();
        }
        if (atomic_load(&button_up_pressed)) {
            atomic_store(&button_up_pressed, false);
            increase_target_temp();
        }

        int heater_output = (int)pid_update(&pid, set_temp, act_temp);
        ESP_LOGI(TAG, "HEATER OUTPUT: %d", heater_output);
        heater_pwm_set(heater_output);
        vTaskDelay(pdMS_TO_TICKS(40));
    }
}
