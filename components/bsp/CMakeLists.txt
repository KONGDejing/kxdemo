set(srcs "src/bsp_port.cpp")

if(CONFIG_BSP_Box3)
    list(APPEND srcs "src/bsp_box3.cpp")
elseif(CONFIG_BSP_Dnesp32S3)
    list(APPEND srcs "src/bsp_dnesp32s3.cpp")
elseif(CONFIG_BSP_KxDemo)
    list(APPEND srcs "src/bsp_kxdemo.cpp")
endif()

idf_component_register(
    SRCS    ${srcs}

    INCLUDE_DIRS "include"

    REQUIRES    nvs_flash
                spiffs
                driver
    
    PRIV_REQUIRES   esp_lcd
)
