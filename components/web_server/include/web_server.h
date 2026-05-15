#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include "esp_http_server.h"

void web_init(void);

httpd_handle_t start_webserver(void);

#endif
