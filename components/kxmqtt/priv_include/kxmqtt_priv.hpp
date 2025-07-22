
#pragma once

#include "kxmqtt.hpp"


#include "esp_log.h"
#include "mqtt_client.h"


#define TAG                     "kxmqtt"


#define BROKER_RECONNECT_MS     5000


#define MQTT_USERNAME           "kxbox"
#define MQTT_PASSWORD           "Rj5HoXYFbxxEIRqCCj6H7jWUV7LGwbpg"
#define MQTT_MESSAGE_QOS        0


extern char client_id[20];
extern char sub_topic[128];
extern char pub_topic[128];
extern esp_mqtt_client_handle_t client_handle;
extern KxMQTTCallback *mqtt_callback;


void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);


void mqtt_set_connetction_status(bool status);
bool mqtt_get_connetction_status();
void mqtt_set_subscription_status(bool status);
bool mqtt_get_subscription_status();
void mqtt_init_client(const char *mac_str);