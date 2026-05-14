#ifndef BUTTONS_H
#define BUTTONS_H

#include <stdatomic.h>
#include "driver/gpio.h"

extern atomic_bool button_down_pressed;
extern atomic_bool button_up_pressed;

void buttons_init(void);

#endif // BUTTONS_H
