idf_component_register(SRCS "web_server.c" "my_time.c" "wifi.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_http_server esp_wifi storage thermostat
                    EMBED_FILES "index.html" "app.js" "favicon.ico")
