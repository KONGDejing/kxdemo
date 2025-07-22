idf_component_register(SRCS "fota.cpp"
                    INCLUDE_DIRS "include"
                    REQUIRES app_update esp_http_client esp_https_ota esp_timer)
