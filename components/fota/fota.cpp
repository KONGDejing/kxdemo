#include <stdio.h>
#include "fota.hpp"

#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "esp_timer.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"


ESP_EVENT_DEFINE_BASE(FOTA_EVENTS);


static const char *TAG = "FOTA";


static esp_app_desc_t running_app_info;
static esp_app_desc_t ota_app_desc;
static char ota_url[128];
static esp_https_ota_handle_t https_ota_handle = NULL;
static TaskHandle_t fota_task_handle = NULL;
static esp_event_loop_handle_t loop_handle = NULL;


static void fota_task(void *args) {
    int total_size = 0;
    int read_size = 0;
    int percent = 0;
    int last_percent = 0;
    esp_err_t err = ESP_OK;

    total_size =  esp_https_ota_get_image_size(https_ota_handle);

    for (;;) {
        err = esp_https_ota_perform(https_ota_handle);
        
        if (err == ESP_ERR_HTTPS_OTA_IN_PROGRESS) {
            read_size = esp_https_ota_get_image_len_read(https_ota_handle);
            
            /* 更新进度 */
            percent = read_size * 100 / total_size;

            if (last_percent != percent) {
                last_percent = percent;

                ESP_LOGI(TAG, "Read %d/%d bytes from partition partition (%d%%)", 
                    read_size, total_size, percent);

                esp_event_post_to(
                    loop_handle,
                    FOTA_EVENTS,
                    FOTA_EVENT_PROGRESS,
                    &percent,
                    sizeof(percent),
                    portMAX_DELAY
                );
            }
        }
        else if (err == ESP_OK) {
            if (!esp_https_ota_is_complete_data_received(https_ota_handle)) {
                break;
            }
            if (esp_https_ota_finish(https_ota_handle) == ESP_OK) {
                /* 更新成功 重启设备 */
                esp_event_post_to(
                    loop_handle,
                    FOTA_EVENTS,
                    FOTA_EVENT_DONE,
                    NULL,
                    0,
                    portMAX_DELAY
                );
                vTaskDelay(2000 / portTICK_PERIOD_MS);//waite play_mp3_file
                esp_restart();
            }
        }
        else {
            break;
        }

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }

    esp_event_post_to(
        loop_handle,
        FOTA_EVENTS,
        FOTA_EVENT_FAILED,
        NULL,
        0,
        portMAX_DELAY
    );

    vTaskDelay(2000 / portTICK_PERIOD_MS);//waite play_mp3_file
    esp_https_ota_abort(https_ota_handle);
    fota_task_handle = NULL;
    vTaskDelete(NULL);

    esp_restart();
}


void Fota::init() {
    const esp_partition_t *running = esp_ota_get_running_partition();
    if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK) {
        
    }

    esp_event_loop_args_t args = {
        .queue_size = 10,
        .task_name = "fota",
        .task_priority = 3,
        .task_stack_size = 4096,
        .task_core_id = 1
    };

    ESP_ERROR_CHECK(esp_event_loop_create(&args, &loop_handle));
}


esp_app_desc_t* Fota::get_app_desc() {
    return &running_app_info;
}


bool Fota::begin(const char *url) {
    if (NULL != fota_task_handle) {
        return false;
    }

    sprintf(ota_url, "%s", url);

    esp_http_client_config_t config = {
        .url = ota_url,
        .method = HTTP_METHOD_GET,
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
        .buffer_size = 1024,
        .skip_cert_common_name_check = true,
        .keep_alive_enable = true,
    };

    esp_https_ota_config_t ota_config = {
        .http_config = &config,
    };

    if (esp_https_ota_begin(&ota_config, &https_ota_handle) == ESP_OK) {
        
        if (esp_https_ota_get_img_desc(https_ota_handle, &ota_app_desc) == ESP_OK) {
            esp_event_post_to(
                loop_handle,
                FOTA_EVENTS,
                FOTA_EVENT_STARTED,
                NULL,
                0,
                portMAX_DELAY
            );
            vTaskDelay(4500 / portTICK_PERIOD_MS);
            xTaskCreatePinnedToCore(
                fota_task, "fota_task", 4096, NULL, 2, &fota_task_handle, 1);
            return true;
        }
    }

    esp_https_ota_abort(https_ota_handle);
    return false;
}


bool Fota::is_in_progress() {
    return (NULL != fota_task_handle);
}


esp_err_t Fota::register_event_handler(esp_event_handler_t event_handler, void *handler_args) {
    return esp_event_handler_register_with(
        loop_handle,
        FOTA_EVENTS,
        FOTA_EVENT_ANY,
        event_handler,
        handler_args
    );
}
