#include "kxmqtt_priv.hpp"


void KxMQTT::init_mqtt(const char *mac_str) {
    mqtt_init_client(mac_str);
}


void KxMQTT::start() {
    if (NULL == client_handle) return;
    esp_mqtt_client_start(client_handle);
}


void KxMQTT::stop() {
    if (NULL == client_handle) return;
    esp_mqtt_client_stop(client_handle);
}


void KxMQTT::subscribe_topic() {
    if (NULL == client_handle) return;
    if (false == mqtt_get_connetction_status()) return;
    if (true == mqtt_get_subscription_status()) return;
    if (strlen(sub_topic) == 0) return;

    esp_mqtt_client_subscribe(client_handle, sub_topic, MQTT_MESSAGE_QOS);
}


int KxMQTT::publish_message(const char *topic, const char *data, int data_len, int qos, bool retain) {
    if (NULL != client_handle) {
        ESP_LOGI(TAG, "Publishing: %s", data);
        return esp_mqtt_client_publish(client_handle, topic, data, data_len, qos, retain);
    }
    return -1;
}


int KxMQTT::publish_message(const char *data, int data_len) {
    return publish_message(pub_topic, data, data_len, MQTT_MESSAGE_QOS, false);
}


int KxMQTT::publish_message(mJSON json_data) {
    return publish_message(json_data, 0);
}


int KxMQTT::clear_broker_down_retain_message(void) {
    if (NULL != client_handle) {
        ESP_LOGI(TAG, "clear broker down retain message");
        return esp_mqtt_client_publish(client_handle, sub_topic, "", 0, MQTT_MESSAGE_QOS, false);
    }
    return -1;
}


void KxMQTT::register_callback(KxMQTTCallback *callback) {
    if (NULL == mqtt_callback) {
        mqtt_callback = callback;
    }
}

const char *KxMQTT::get_client_id() {
    return client_id;
}