#include "driver/i2c_master.h"
#include "esp_log.h"
#include "display.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "main";

#define I2C_SDA 8
#define I2C_SCL 9

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
	i2c_init();
    display_init(i2c_bus_handle, 15, 25);
    int actual = 15;
    int add = -1;
    vTaskDelay(pdMS_TO_TICKS(5000));
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
        display_set_actual_temp(actual + add);
        add *= -1;
    }
}
