#include "driver/i2c_master.h"

#define MIN_TARGET_TEMP 0
#define MAX_TARGET_TEMP 40
#define TARGET_TEMP_CHANGE_VALUE 0.5

#define BMP280_ADDRESS 0x76

#define HEATER_PWM_PIN GPIO_NUM_5

void thermostat_init(
	i2c_master_bus_handle_t bus_handle,
	float actual_temp,
	float target_temp
);

typedef struct {
    float Kp;
    float Ki;
    float Kd;

    float integral;
    float prev_error;
} PID;

void heater_pwm_init(void);

float get_heater_pwm();
void set_heater_pwm(float pwm);
float get_actual_temp();
void set_actual_temp(float temp);
float get_target_temp();
void set_target_temp(float temp);
bool increase_target_temp();
bool decrease_target_temp();
