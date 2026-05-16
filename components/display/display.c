#include "display.h"
#include "stdio.h"
#include "esp_err.h"
#include "esp_log.h"

#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_ssd1306.h"
#include "lvgl.h"
#include "esp_lvgl_port.h"

static const char *TAG = "SSD1306_DISPLAY";

#define SSD1306_WIDTH   128
#define SSD1306_HEIGHT   64

#define SSD1306_ADDRESS 0x3C

/* how bytes are transported to display */
static esp_lcd_panel_io_handle_t disp_io_handle = NULL;
/* actual display driver instance */
static esp_lcd_panel_handle_t disp_panel_handle = NULL;
static lv_display_t *display;

lv_obj_t *actual_temp_label = NULL;
lv_obj_t *target_temp_label = NULL;

static void create_ui(void)
{
	ESP_LOGI(TAG, "Creating UI");
	lvgl_port_lock(0);
	lv_obj_t *root = lv_obj_create(lv_screen_active());
	lv_obj_remove_style_all(root);
	lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));

	lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(
		root,
		LV_FLEX_ALIGN_START,
		LV_FLEX_ALIGN_CENTER,
		LV_FLEX_ALIGN_CENTER
	);

	lv_obj_t *top = lv_obj_create(root);
	lv_obj_remove_style_all(top);
	lv_obj_set_width(top, LV_PCT(100));
	lv_obj_set_height(top, LV_PCT(50));
	actual_temp_label = lv_label_create(top);
	lv_obj_center(actual_temp_label);

	lv_obj_t *bottom = lv_obj_create(root);
	lv_obj_remove_style_all(bottom);
	lv_obj_set_width(bottom, LV_PCT(100));
	lv_obj_set_height(bottom, LV_PCT(50));
	target_temp_label = lv_label_create(bottom);
	lv_obj_center(target_temp_label);

	lvgl_port_unlock();
}

void display_init(
	i2c_master_bus_handle_t bus_handle,
	float actual_temp,
	float target_temp
) {
	ESP_LOGI(TAG, "Initializing SSD1306 display");
    esp_lcd_panel_io_i2c_config_t io_config = {
        .dev_addr = SSD1306_ADDRESS,
        .scl_speed_hz = 400000,
        .control_phase_bytes = 1,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .dc_bit_offset = 6,
    };

	ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(
		bus_handle,
		&io_config,
		&disp_io_handle
    ));

	esp_lcd_panel_dev_config_t panel_config = {
        .bits_per_pixel = 1,
        .reset_gpio_num = -1,
    };
    esp_lcd_panel_ssd1306_config_t ssd1306_config = {
        .height = SSD1306_HEIGHT,
    };
    panel_config.vendor_config = &ssd1306_config;

    ESP_ERROR_CHECK(esp_lcd_new_panel_ssd1306(
		disp_io_handle,
		&panel_config,
		&disp_panel_handle
    ));

	ESP_ERROR_CHECK(esp_lcd_panel_reset(disp_panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(disp_panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(disp_panel_handle, true));

	ESP_LOGI(TAG, "Intializing LVGL");
	const lvgl_port_cfg_t lvgl_cfg = {
		.task_priority = 4,
		.task_stack = 8192,
		.task_affinity = -1,
		.task_max_sleep_ms = 500,
		.timer_period_ms = 5
	};

	ESP_ERROR_CHECK(lvgl_port_init(&lvgl_cfg));

	const lvgl_port_display_cfg_t disp_cfg = {
		.io_handle = disp_io_handle,
		.panel_handle = disp_panel_handle,
		.buffer_size = SSD1306_WIDTH * SSD1306_HEIGHT,
		.double_buffer = false,
		.hres = SSD1306_WIDTH,
		.vres = SSD1306_HEIGHT,
		.monochrome = true,
		.color_format = LV_COLOR_FORMAT_I1,
		.rotation = {
			.swap_xy = false,
			.mirror_x = true,
			.mirror_y = true,
		}
	};
	display = lvgl_port_add_disp(&disp_cfg);
	if (display == NULL) {
		ESP_LOGE(TAG, "Could not intialize LVGL display with esp_lvgl_port");
		return;
	}

	create_ui();
	display_set_actual_temp(actual_temp);
	display_set_target_temp(target_temp);
}

void display_set_actual_temp(float temp)
{
	char buffer[64];
	snprintf(buffer, sizeof(buffer), "Actual: %.2f C", temp);

	lvgl_port_lock(0);
	lv_label_set_text(actual_temp_label, buffer);
	lvgl_port_unlock();
}

void display_set_target_temp(float temp)
{
	char buffer[64];
	snprintf(buffer, sizeof(buffer), "Target: %.2f C", temp);

	lvgl_port_lock(0);
	lv_label_set_text(target_temp_label, buffer);
	lvgl_port_unlock();
}
