idf_component_register(SRCS "web_server.c" "wifi.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_http_server esp_wifi storage thermostat my_time
                    EMBED_FILES "index.html" "app.js" "favicon.ico")
