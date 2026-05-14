#include "driver/i2c_master.h"
#include "esp_log.h"
#include "display.h"
#include "thermostat.h"
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

    while (1) {
        display_set_actual_temp(get_actual_temp());
        display_set_target_temp(get_target_temp());
        vTaskDelay(pdMS_TO_TICKS(30));
    }
}
