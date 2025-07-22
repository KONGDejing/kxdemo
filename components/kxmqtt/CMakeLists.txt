idf_component_register(
    SRCS    "kxmqtt.cpp"
            "src/callback.cpp"
            "src/client.cpp"
    
    INCLUDE_DIRS "include"

    PRIV_INCLUDE_DIRS "priv_include"

    REQUIRES    mqtt

    PRIV_REQUIRES   kxutils

#     EMBED_TXTFILES  cert.key
#                     cert.pem
)
