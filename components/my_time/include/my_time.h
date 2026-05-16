#ifndef MY_TIME_H
#define MY_TIME_H

#include "esp_sntp.h"
#include "time.h"
#include "stdatomic.h"

extern atomic_bool is_time_set_up;

void init_sntp(void);

void get_time(struct tm *timeinfo);

void time_to_str(struct tm *timeinfo, char buffer[64]);

#endif // MY_TIME_H
