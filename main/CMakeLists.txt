idf_component_register(
    SRCS    "main.cpp"
            "src/app_callbacks.cpp"
            "src/dto.cpp"
            "src/blink.cpp"

    INCLUDE_DIRS    "."
                    "include"

    PRIV_REQUIRES   bsp
                    netcm
                    fota
                    kxchat
                    kxutils
                    kxmqtt
                    kxbt
                    kxfiles
                    
                    esp_timer
)
