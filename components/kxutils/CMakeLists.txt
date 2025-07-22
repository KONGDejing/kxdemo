idf_component_register(
    SRCS    "src/kxutils.cpp"
            "src/websocket.cpp"
            "src/nvs_store.cpp"
    
    INCLUDE_DIRS "include"

    REQUIRES    json
                nvs_flash
                esp_websocket_client
)
