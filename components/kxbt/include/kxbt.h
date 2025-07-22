
#ifndef __KXBT_H__
#define __KXBT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "esp_err.h"
#include "esp_check.h"
#include "esp_blufi.h"
#include "esp_blufi_api.h"

esp_err_t kxbt_init(void);
esp_err_t kxbt_deinit(void);
esp_err_t kxbt_blufi_init(esp_blufi_event_cb_t event_cb);

#ifdef __cplusplus
}
#endif

#endif  // __KXBT_H__
