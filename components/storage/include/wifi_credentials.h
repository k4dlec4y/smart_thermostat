#ifndef WIFI_CREDENTIALS_H
#define WIFI_CREDENTIALS_H

/*
these credentials are used only if FORCE_NEW_WIFI is true,
then they are saved to the NVS partition
*/

#define WIFI_SSID     "my_wifi"
#define WIFI_PASSWORD "supersecret"

#define FORCE_NEW_WIFI    true

#endif // WIFI_CREDENTIALS_H
