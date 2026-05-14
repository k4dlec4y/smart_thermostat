idf_component_register(SRCS "buttons.c" "thermostat.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_driver_i2c esp_driver_gpio k0i05__esp_bmp280)
