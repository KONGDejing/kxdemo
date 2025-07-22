#include "kxutils/websocket.hpp"

#include "esp_log.h"
#include "esp_timer.h"


static void websocket_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    WebSocket *ws = (WebSocket*)handler_args;
    esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;

    switch (event_id) {
        case WEBSOCKET_EVENT_ERROR:
            break;
        
        case WEBSOCKET_EVENT_CONNECTED:
            ESP_LOGI(ws->get_name(), "websocket connected");
            break;

        case WEBSOCKET_EVENT_DISCONNECTED:
            ESP_LOGI(ws->get_name(), "websocket disconnected");
            break;

        case WEBSOCKET_EVENT_DATA:
            if (data->op_code == 0x0) {
                ESP_LOGI(ws->get_name(), "Received %d %s", data->data_len, (char *)data->data_ptr);
            }
            else if (data->op_code == 0x1) {
                if (data->data_len > 0) {
                    char *temp = (char*)data->data_ptr;
                    temp[data->data_len] = '\0';
                    ESP_LOGI(ws->get_name(), "Received text %d %s", data->data_len, (char *)data->data_ptr);
                    ws->on_string_message(data);
                }
                else {
                    ESP_LOGI(ws->get_name(), "Received empty text message");
                }
            }
            else if (data->op_code == 0x2) { // Opcode 0x2 indicates binary data
                // ESP_LOGI(ws->get_name(), "Received binary data %d payload_len: %d payload_offset: %d", 
                //     data->data_len, data->payload_len, data->payload_offset);
                ws->on_bytes_message(data);
            } 
            else if (data->op_code == 0x08) {
                if (data->data_len == 2) {
                    ESP_LOGW(ws->get_name(), "Received closed message with code=%d", 256 * data->data_ptr[0] + data->data_ptr[1]);
                } else {
                    ESP_LOGW(ws->get_name(), "Received closed message with unknown code=%d", data->data_len);
                }
            }
            else if (data->op_code == 0x09) {
                ESP_LOGI(ws->get_name(), "Received ping message");
            }
            else if (data->op_code == 0x0A) {
                ESP_LOGI(ws->get_name(), "Received pong message");
            }
            else {
                ESP_LOGI(ws->get_name(), "Received unknown opcode %d", data->op_code);                   
            }
            break;

        case WEBSOCKET_EVENT_CLOSED:
            ESP_LOGI(ws->get_name(), "websocket closed");
            ws->destroy();
            break;

        case WEBSOCKET_EVENT_BEFORE_CONNECT:
            break;
    }
}


WebSocket::WebSocket() {
    event_group = xEventGroupCreate();
    tag = strdup("websocket");
}


WebSocket::WebSocket(const char *name) {
    event_group = xEventGroupCreate();
    tag = strdup(name);
}


WebSocket::~WebSocket() {
    if (NULL!= ws_handle) {
        esp_websocket_client_destroy(ws_handle);
        ws_handle = NULL;
    }
    if (NULL!= tag) {
        free(tag);
        tag = NULL;
    }
    if (NULL!= event_group) {
        vEventGroupDelete(event_group);
        event_group = NULL;
    }
}


void WebSocket::init_ws() {
    if (NULL != ws_handle) return;

    esp_websocket_client_config_t ws_cfg = {
        .uri = get_ws_url(),
        // .disable_auto_reconnect = true,
        .task_prio = 2,
        .task_stack = 1024 * 8,
        // .buffer_size = 81920,
        .transport = WEBSOCKET_TRANSPORT_OVER_SSL,
        .skip_cert_common_name_check = true,
        .reconnect_timeout_ms = 5 * 1000,
        .network_timeout_ms = 10 * 1000
    };
    ws_handle = esp_websocket_client_init(&ws_cfg);
    esp_websocket_register_events(ws_handle, WEBSOCKET_EVENT_ANY, websocket_event_handler, (void *)this);
    esp_websocket_client_start(ws_handle);
}


void WebSocket::start_ws() {
    esp_websocket_client_start(ws_handle);
    ESP_LOGI(get_name(), "websocket started");
}


void WebSocket::wait_ws_connected() {
    int64_t start = esp_timer_get_time();
    if (!esp_websocket_client_is_connected(ws_handle)) {
        ESP_LOGI(get_name(), "websocket not connected, start it");
        start_ws();
    }

    while (!esp_websocket_client_is_connected(ws_handle))
    {
        vTaskDelay(100 / portTICK_PERIOD_MS);
        if (esp_timer_get_time() - start > 20 * 1000 * 1000) {
            ESP_LOGE(get_name(), "websocket connect timeout");
            break;
        }
    }
}


void WebSocket::close() {
    if (NULL!= ws_handle) {
        esp_websocket_client_close(ws_handle, 1000);
        ESP_LOGI(get_name(), "websocket client closed");
    }
}


void WebSocket::destroy() {
    if (NULL!= ws_handle) {
        esp_websocket_client_stop(ws_handle);
        ESP_LOGI(get_name(), "websocket client stopped");
        esp_websocket_client_destroy_on_exit(ws_handle);
        ESP_LOGI(get_name(), "websocket destroyed");
        ws_handle = NULL;
    }
}


void WebSocket::new_session() {
    if (NULL == ws_handle) {
        init_ws();
    }
    wait_ws_connected();
}


bool WebSocket::is_connected() {
    return NULL != ws_handle;
}


int WebSocket::ws_send_text(const char *data, int len, TickType_t timeout) {
    if (len > 1024) {
        ESP_LOGI(get_name(), "long message: %d", len);
    } else {
        ESP_LOGI(get_name(), "Sending message: %s", data);
    }
    return esp_websocket_client_send_text(ws_handle, data, len, timeout);
}


int WebSocket::ws_send_binary(const char *data, int len, TickType_t timeout) {
    // ESP_LOGI(get_name(), "Sending binary message: %d", len);
    return esp_websocket_client_send_bin(ws_handle, data, len, timeout);
}
