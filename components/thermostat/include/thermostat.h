#ifndef THERMOSTAT_H
#define THERMOSTAT_H

#include "driver/i2c_master.h"

/**
 * Initialize BMP280 sensor and PWM output
 */
void thermostat_init(
	i2c_master_bus_handle_t bus_handle,
	float actual_temp,
	float target_temp
);

float get_heater_pwm();
void set_heater_pwm(float pwm);

float get_actual_temp();
void set_actual_temp(float temp);

float get_target_temp();
void set_target_temp(float temp);
bool increase_target_temp();
bool decrease_target_temp();

#endif // THERMOSTAT_H
