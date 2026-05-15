#ifndef STORAGE_H
#define STORAGE_H

extern const char *OLD_LOG_FILE;
extern const char *LOG_FILE;

typedef struct {
    char ssid[32];
    char password[64];
} wifi_credentials_t;

void storage_init(void);
void start_logger_task(void);

bool storage_has_wifi_credentials(void);
wifi_credentials_t storage_get_wifi_credentials(void);

float storage_get_target_temp(void);
void storage_set_target_temp(float temp);

#endif // STORAGE_H
