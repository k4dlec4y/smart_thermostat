#ifndef WIFI_H
#define WIFI_H

#include <stdatomic.h>

extern atomic_bool ip_changed;
extern char ip_address[64];

void wifi_init(void);

#endif // WIFI_H
