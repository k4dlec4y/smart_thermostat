idf_component_register(SRCS "thermostat.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_driver_i2c k0i05__esp_bmp280)
