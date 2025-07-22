
#ifndef __KXFILES_H__
#define __KXFILES_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "esp_err.h"


typedef enum {
    KXFILES_BO_MP3 = 0,
    KXFILES_XIU_MP3,

    KXFILES_NO_WIFI_MP3,
    KXFILES_WIFI_CONNECT_FAIL_MP3,
    KXFILES_WIFI_CONNECTING_MP3,
    KXFILES_WIFI_CONNECT_SUCCESS_MP3,
    KXFILES_START_OTA_MP3,
    KXFILES_OTA_SUCCESS_MP3,
    KXFILES_OTA_FAIL_MP3,
    
    KXFILES_MAX
} kxfiles_file_id_t;


typedef struct
{
    const uint8_t *start;
    const uint8_t *end;
    int size;
} file_info_t;


/**
 * 
 * 通过文件ID获取文件信息
 * 
 * @param file_id 文件ID
 * @param file_info 文件信息
 * @return
 * - ESP_OK 成功
 * - ESP_ERR_INVALID_ARG 参数错误
 * 
 */
esp_err_t kxfiles_get_file_info(kxfiles_file_id_t file_id, file_info_t *file_info);


#ifdef __cplusplus
}
#endif

#endif // __KXFILES_H__
