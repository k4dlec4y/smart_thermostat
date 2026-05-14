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

float pid_update(PID *pid, float set_temp, float act_temp);

void heater_pwm_init(void);
void heater_pwm_set(int duty_percentage);

float get_actual_temp();
void set_actual_temp(float temp);
float get_target_temp();
void set_target_temp(float temp);
bool increase_target_temp();
bool decrease_target_temp();
