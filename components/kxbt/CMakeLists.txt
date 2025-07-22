idf_component_register(
    SRCS    "src/kxbt.c"
            "src/blufi_security.c"
            "src/blufi_init.c"
    
    INCLUDE_DIRS "include"

    PRIV_INCLUDE_DIRS "priv_include"

    REQUIRES    bt
                mbedtls
)
