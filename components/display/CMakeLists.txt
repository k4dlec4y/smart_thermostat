idf_component_register(SRCS "display.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_lcd esp_driver_i2c espressif__esp_lvgl_port)
