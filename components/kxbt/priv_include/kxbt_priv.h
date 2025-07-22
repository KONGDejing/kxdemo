
#ifndef __KXBT_PRIV_H__
#define __KXBT_PRIV_H__

#include "kxbt.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_err.h"
#include "esp_log.h"
#include "esp_blufi_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/* blufi_init */
esp_err_t esp_blufi_controller_init();
esp_err_t esp_blufi_controller_deinit();
esp_err_t esp_blufi_host_and_cb_init(esp_blufi_callbacks_t *example_callbacks);

/* blufi security */
esp_err_t blufi_security_init(void);
void blufi_security_deinit(void);
void blufi_dh_negotiate_data_handler(uint8_t *data, int len, 
    uint8_t **output_data, int *output_len, bool *need_free);
int blufi_aes_encrypt(uint8_t iv8, uint8_t *crypt_data, int crypt_len);
int blufi_aes_decrypt(uint8_t iv8, uint8_t *crypt_data, int crypt_len);
uint16_t blufi_crc_checksum(uint8_t iv8, uint8_t *data, int len);

#ifdef __cplusplus
}
#endif


#endif // __KXBT_PRIV_H__
