
#pragma once


#include "kxutils/cxxjson.hpp"


class KxMQTTCallback {
public:
    virtual void on_connected(void) = 0;
    virtual void on_disconnected(void) = 0;
    virtual void on_message(const char *data, int data_len) = 0;

private:

protected:

};


class KxMQTT {
public:
    static void init_mqtt(const char *mac_str);
    static void start();
    static void stop();
    static void subscribe_topic();

    static const char *get_client_id();
    
    static int publish_message(const char *topic, const char *data, int data_len, int qos, bool retain);
    static int publish_message(const char *data, int data_len);
    static int publish_message(mJSON json_data);
    
    static int clear_broker_down_retain_message(void);

    static void register_callback(KxMQTTCallback *callback);
    
private:

protected:

};
