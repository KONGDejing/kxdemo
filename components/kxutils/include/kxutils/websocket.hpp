
#pragma once

#ifndef __KX_WEBSOCKET_HPP__
#define __KX_WEBSOCKET_HPP__


#include <stdio.h>
#include <stdint.h>
#include <string.h>


#include "esp_websocket_client.h"


class WebSocket {
public:
    WebSocket();
    WebSocket(const char *name);
    ~WebSocket();

    void new_session();
    bool is_connected();
    int ws_send_text(const char *data, int len, TickType_t timeout);
    int ws_send_binary(const char *data, int len, TickType_t timeout);
    void close();
    void destroy();

    const char *get_name() { return tag; }

    virtual const char *get_ws_url() = 0;
    
    virtual void on_bytes_message(esp_websocket_event_data_t *event_data) { }
    virtual void on_string_message(esp_websocket_event_data_t *event_data) { }

private:
    esp_websocket_client_handle_t ws_handle = NULL;

    char *tag = NULL;

    void init_ws();
    void start_ws();
    void wait_ws_connected();

protected:
    EventGroupHandle_t event_group = NULL;
    char *binary_buffer = NULL;

};


#endif /* __KX_WEBSOCKET_HPP__ */
