#ifndef DISPLAY_H
#define DISPLAY_H

#include "driver/i2c_master.h"

void display_init(
	i2c_master_bus_handle_t bus_handle,
	float actual_temp,
	float target_temp
);

void display_set_actual_temp(float temp);

void display_set_target_temp(float temp);

#endif // DISPLAY_H
