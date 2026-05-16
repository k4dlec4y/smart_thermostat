#include "driver/i2c_master.h"
#include "esp_log.h"
#include "display.h"
#include "thermostat.h"
#include "buttons.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "storage.h"
#include "web_server.h"
#include "wifi.h"

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
    storage_init();
    float default_actual_temp = 15;
    float default_target_temp = storage_get_target_temp();

	i2c_init();
    thermostat_init(i2c_bus_handle, default_actual_temp, default_target_temp);
    display_init(i2c_bus_handle, default_actual_temp, default_target_temp);
    buttons_init();
    web_init();
    start_logger_task();

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
        if (atomic_load(&ip_changed)) {
            atomic_store(&ip_changed, false);
            display_set_ip_address(ip_address);
        }
        if (atomic_load(&g_save_target_temp)) {
            atomic_store(&g_save_target_temp, false);
            storage_set_target_temp(set_temp);
        }

        vTaskDelay(pdMS_TO_TICKS(40));
    }
}
