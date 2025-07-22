#include "kxmqtt_priv.hpp"


// extern const char cert_key_start[] asm("_binary_cert_key_start");
// extern const char cert_key_end[] asm("_binary_cert_key_end");

// extern const char cert_pem_start[] asm("_binary_cert_pem_start");
// extern const char cert_pem_end[] asm("_binary_cert_pem_end");


static bool is_connected = false;
static bool is_subscribed = false;


char client_id[20] = {0};
char sub_topic[128] = {0};
char pub_topic[128] = {0};
esp_mqtt_client_handle_t client_handle = NULL;
KxMQTTCallback *mqtt_callback = NULL;


void mqtt_set_connetction_status(bool status) {
    is_connected = status;
}


bool mqtt_get_connetction_status() {
    return is_connected;
}


void mqtt_set_subscription_status(bool status) {
    is_subscribed = status;
}


bool mqtt_get_subscription_status() {
    return is_subscribed;
}


void mqtt_init_client(const char *mac_str) {
    sprintf(client_id, "%s", mac_str);

    ESP_LOGI(TAG, "Client ID: %s", client_id);
    sprintf(sub_topic, "down/kxbox/%s", client_id);
    ESP_LOGI(TAG, "Subscribe topic: %s", sub_topic);
    sprintf(pub_topic, "up/kxbox/%s", client_id);
    ESP_LOGI(TAG, "Publish topic: %s", pub_topic);

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker = {
            .address = {
                .uri = "wss://emqxsl.kaixinyongyuan.cn/mqtt"
            },
        },
        .credentials = {
            .username = MQTT_USERNAME,
            .client_id = client_id,
            .set_null_client_id = false,
            .authentication = {
                .password = MQTT_PASSWORD,
                // .certificate = cert_pem_start,
                // .certificate_len = (size_t)(cert_pem_end - cert_pem_start),
                // .key = cert_key_start,
                // .key_len = (size_t)(cert_key_end - cert_key_start),
            }
        },
        .session = {
            .disable_clean_session = false,
            .keepalive = 120,
            .protocol_ver = MQTT_PROTOCOL_V_3_1_1
        },
        .network = {
            .reconnect_timeout_ms = BROKER_RECONNECT_MS,
            .timeout_ms = BROKER_RECONNECT_MS,
            .disable_auto_reconnect = false,
        },
        .task = {
            .priority = 5,
            .stack_size = 4096
        },
        .buffer = {
            .size = 1024,
            .out_size = 1024
        }
    };

    client_handle = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client_handle, MQTT_EVENT_ANY, mqtt_event_handler, NULL);
}