
#pragma once


#include "esp_err.h"
#include "esp_log.h"
#include "nvs.h"


#define NVS_NAMESPACE "nvs_data"


#define RETURN_IF_ERROR(err) if (err != ESP_OK) { return err; }


// static const char *TAG = "NvsStore";


class NvsStore {
public:
    static esp_err_t find_key(const char *key) {
        nvs_handle_t nvs_handle;
        esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
        RETURN_IF_ERROR(err);
        err = nvs_find_key(nvs_handle, key, NULL);
        nvs_close(nvs_handle);
        return err;
    }

    static esp_err_t number_set_int32(const char *key, int32_t value) {
        nvs_handle_t nvs_handle;
        esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
        RETURN_IF_ERROR(err);
        err = nvs_set_i32(nvs_handle, key, value);
        RETURN_IF_ERROR(err);
        err = nvs_commit(nvs_handle);
        nvs_close(nvs_handle);
        return err;
    }

    static esp_err_t number_get_int32(const char *key, int32_t *value) {
        nvs_handle_t nvs_handle;
        esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
        RETURN_IF_ERROR(err);
        err = nvs_get_i32(nvs_handle, key, value);
        nvs_close(nvs_handle);
        return err;
    }

private:

protected:

};

#undef RETURN_IF_ERROR