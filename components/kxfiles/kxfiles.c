#include "kxfiles.h"


#define __STR__(x)                          #x
#define BINARY_FILE_START(x)                x##_start
#define BINARY_FILE_END(x)                  x##_end
#define BINARY_FILE_SIZE(x)                 (x##_end - x##_start)
#define BINARY_FILE_DECLARATE_START(x)      extern const uint8_t x##_start[] asm(__STR__(_binary_##x##_start));
#define BINARY_FILE_DECLARATE_END(x)        extern const uint8_t x##_end[] asm(__STR__(_binary_##x##_end));
#define BINARY_FILE_DECLARATE(x)            BINARY_FILE_DECLARATE_START(x) \
                                            BINARY_FILE_DECLARATE_END(x)
#define BINARY_FILE_INFO(x)                 (file_info_t){BINARY_FILE_START(x), BINARY_FILE_END(x), BINARY_FILE_SIZE(x)}


// Binary files
BINARY_FILE_DECLARATE(bo_mp3)
BINARY_FILE_DECLARATE(xiu_mp3)

BINARY_FILE_DECLARATE(no_wifi_mp3)
BINARY_FILE_DECLARATE(wifi_connect_fail_mp3)
BINARY_FILE_DECLARATE(wifi_connecting_mp3)
BINARY_FILE_DECLARATE(wifi_connect_success_mp3)
BINARY_FILE_DECLARATE(start_ota_mp3)
BINARY_FILE_DECLARATE(ota_success_mp3)
BINARY_FILE_DECLARATE(ota_fail_mp3)


esp_err_t kxfiles_get_file_info(kxfiles_file_id_t file_id, file_info_t *file_info) {
    if (!file_info) {
        return ESP_ERR_INVALID_ARG;
    }

    switch (file_id) {
        case KXFILES_BO_MP3:
            *file_info = BINARY_FILE_INFO(bo_mp3);
            return ESP_OK;
        case KXFILES_XIU_MP3:
            *file_info = BINARY_FILE_INFO(xiu_mp3);
            return ESP_OK;
        case KXFILES_NO_WIFI_MP3:
            *file_info = BINARY_FILE_INFO(no_wifi_mp3);
            return ESP_OK;
        case KXFILES_WIFI_CONNECT_FAIL_MP3:
            *file_info = BINARY_FILE_INFO(wifi_connect_fail_mp3);
            return ESP_OK;
        case KXFILES_WIFI_CONNECTING_MP3:
            *file_info = BINARY_FILE_INFO(wifi_connecting_mp3);
            return ESP_OK;
        case KXFILES_WIFI_CONNECT_SUCCESS_MP3:
            *file_info = BINARY_FILE_INFO(wifi_connect_success_mp3);
            return ESP_OK;
        case KXFILES_START_OTA_MP3:
            *file_info = BINARY_FILE_INFO(start_ota_mp3);
            return ESP_OK;
        case KXFILES_OTA_SUCCESS_MP3:
            *file_info = BINARY_FILE_INFO(ota_success_mp3);
            return ESP_OK;
        case KXFILES_OTA_FAIL_MP3:
            *file_info = BINARY_FILE_INFO(ota_fail_mp3);
            return ESP_OK;
        default:
            return ESP_ERR_INVALID_ARG;
    }
}
