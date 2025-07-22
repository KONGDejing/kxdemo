
#pragma once

#include <stdint.h>

#include "esp_event.h"

#include "netcm.hpp"
#include "kxmqtt.hpp"


class AppNetcmCallback : public NetcmCallback {
public:
    void on_netif_got_ip(const char *ip);
    void on_netif_lost_ip();

    void on_sta_start_scan(uint8_t *ssid);
    void on_sta_scan_done(wifi_ap_record_t *ap_list, uint16_t ap_num);

    void on_sta_start_connect(uint8_t *ssid);
    void on_sta_connect_success(wifi_event_sta_connected_t *event_data);
    void on_sta_connect_failed(wifi_event_sta_disconnected_t *event_data);

    void on_sta_connected(uint8_t *ssid, uint8_t *bssid);
    void on_sta_disconnected(uint8_t *ssid, int32_t rssi, uint8_t reason);

    void on_netcm_heart_handle();

private:

protected:

};


class AppMqttCallback : public KxMQTTCallback {
public:
    void on_connected(void);
    virtual void on_disconnected(void);
    virtual void on_message(const char *data, int data_len);

private:

protected:

};


void kx_chat_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data);
void button_play_event_cb(void *arg, void *data);
void app_ota_progress_cb(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
void kx_set_volume_event_handler(void* arg, esp_event_base_t event_base,
    int32_t event_id, void* event_data);