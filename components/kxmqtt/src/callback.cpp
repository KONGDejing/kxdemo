#include "kxmqtt_priv.hpp"


void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;

    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_ERROR:

            break;

        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "Connected to MQTT broker");

            mqtt_set_connetction_status(true);
            
            if (NULL != mqtt_callback) {
                mqtt_callback->on_connected();
            }

            KxMQTT::subscribe_topic();
            break;
        
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "Disconnected from MQTT broker");

            mqtt_set_connetction_status(false);

            if (NULL != mqtt_callback) {
                mqtt_callback->on_disconnected();
            }
            break;
        
        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "Subscribe success");
            mqtt_set_subscription_status(true);
            break;
        
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "Unsubscribe success");
            mqtt_set_subscription_status(false);
            break;
        
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "Publish success, msg_id: %d", event->msg_id);
            break;
        
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "Receive %d", event->data_len);
            if (event->data_len > 0) {
                if (event->data_len < strlen(event->data)) {
                    (event->data)[event->data_len] = '\0'; // ensure null-terminated string [0, data_len-1, '\0' if data_len > 0, '\0' if data_
                }
                if (NULL != mqtt_callback) {
                    mqtt_callback->on_message(event->data, event->data_len);
                }
            } else {
                ESP_LOGI(TAG, "Receive empty message");
            }
            break;
        
        case MQTT_EVENT_BEFORE_CONNECT:
            ESP_LOGI(TAG, "Connecting to MQTT broker...");
            break;
        
        case MQTT_EVENT_DELETED:
            ESP_LOGI(TAG, "Client deleted");
            break;
        
        case MQTT_USER_EVENT:
            break;

        default:
            break;
    }
}
