#include "kxbt_priv.h"


// static const char TAG[] = "kxbt";


esp_err_t kxbt_init(void) {
    blufi_security_init();
    return esp_blufi_controller_init();
}


esp_err_t kxbt_deinit(void) {
    blufi_security_deinit();
    return esp_blufi_controller_deinit();
}


esp_err_t kxbt_blufi_init(esp_blufi_event_cb_t event_cb) {
    esp_blufi_callbacks_t _callbacks = {
        .event_cb = event_cb,
        .negotiate_data_handler = blufi_dh_negotiate_data_handler,
        .encrypt_func = blufi_aes_encrypt,
        .decrypt_func = blufi_aes_decrypt,
        .checksum_func = blufi_crc_checksum,
    };
    return esp_blufi_host_and_cb_init(&_callbacks);
}
