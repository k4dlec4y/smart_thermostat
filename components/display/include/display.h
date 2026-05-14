#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_ssd1306.h"

#include "lvgl.h"
#include "esp_lvgl_port.h"

void display_init(
	i2c_master_bus_handle_t bus_handle,
	float actual_temp,
	float target_temp
);

void display_set_actual_temp(float temp);

void display_set_target_temp(float temp);
