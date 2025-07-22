idf_component_register(
    SRCS    "src/netcm.cpp"
            "src/sntp.cpp"
            "src/sta_scan.cpp"
            "src/ping.cpp"
    
    INCLUDE_DIRS "include"

    PRIV_INCLUDE_DIRS "priv_include"
    
    REQUIRES    nvs_flash
                json
                esp_wifi
                esp_timer

    PRIV_REQUIRES   bt
                    esp_http_server
    
    EMBED_TXTFILES index.html
)
