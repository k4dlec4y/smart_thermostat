#include "driver/i2c_master.h"

#define MIN_TARGET_TEMP 0
#define MAX_TARGET_TEMP 40
#define TARGET_TEMP_CHANGE_VALUE 0.5

#define BMP280_ADDRESS 0x76

void thermostat_init(
	i2c_master_bus_handle_t bus_handle,
	float actual_temp,
	float target_temp
);

float get_actual_temp();
void set_actual_temp(float temp);
float get_target_temp();
void set_target_temp(float temp);
bool increase_target_temp();
bool decrease_target_temp();
