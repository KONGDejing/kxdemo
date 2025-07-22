idf_component_register(
    SRCS    "src/kxaudio.cpp" 
            "src/kx_audio_types.cpp"
            "src/kxopus.cpp"
            "src/kxsession.cpp"
            "src/kxchat.cpp"

    INCLUDE_DIRS    "include"

    REQUIRES    esp_http_client 
                mbedtls 
                esp_timer

    PRIV_REQUIRES   kxutils
)
