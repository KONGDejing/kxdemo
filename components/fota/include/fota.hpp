
#ifndef __FOT_H__
#define __FOT_H__

#include "esp_event.h"
#include "esp_ota_ops.h"

typedef enum {
    FOTA_EVENT_ANY = -1,
    FOTA_EVENT_STARTED,
    FOTA_EVENT_PROGRESS,
    FOTA_EVENT_DONE,
    FOTA_EVENT_FAILED,
} fota_event_id_t;


class Fota {
public:
    static void init();
    static esp_app_desc_t *get_app_desc();
    static bool begin(const char *url);
    static bool is_in_progress();
    static esp_err_t register_event_handler(esp_event_handler_t event_handler, void *handler_args);

private:

};

#endif // __FOT_H__
