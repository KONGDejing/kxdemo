

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "esp_err.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_blufi_api.h"


#define MAX_SOTRE_SIZE 20


typedef struct {
    char ssid[32];
    char password[64];
}sta_auth_info_t;


typedef struct {
    bool is_connected;
    wifi_event_sta_connected_t *sta_connected_event;
    wifi_event_sta_disconnected_t *sta_disconnected_event;

    void clear() {
        if (sta_connected_event!= NULL) {
            free(sta_connected_event);
            sta_connected_event = NULL;
        }
        if (sta_disconnected_event!= NULL) {
            free(sta_disconnected_event);
            sta_disconnected_event = NULL;
        }
        is_connected = false;
    }

    bool is_empty() {
        if (NULL != sta_connected_event) return false;
        if (NULL != sta_disconnected_event) return false;
        return true;
    }

    void set_sta_connected(wifi_event_sta_connected_t *event) {
        if (!is_empty()) return;

        is_connected = true;
        sta_connected_event = (wifi_event_sta_connected_t*)malloc(sizeof(wifi_event_sta_connected_t));
        memset(sta_connected_event, 0x0, sizeof(wifi_event_sta_connected_t));
        memcpy(sta_connected_event, event, sizeof(wifi_event_sta_connected_t));
    }

    void set_sta_disconnected(wifi_event_sta_disconnected_t *event) {
        if (!is_empty()) return;

        is_connected = false;
        sta_disconnected_event = (wifi_event_sta_disconnected_t*)malloc(sizeof(wifi_event_sta_disconnected_t));
        memset(sta_disconnected_event, 0x0, sizeof(wifi_event_sta_disconnected_t));
        memcpy(sta_disconnected_event, event, sizeof(wifi_event_sta_disconnected_t));
    }
}sta_conn_result_t;


class NetcmCallback {
public:
    virtual void on_netif_got_ip(const char *ip) { }
    virtual void on_netif_lost_ip() { }

    virtual void on_sta_start_scan(uint8_t *ssid) { }
    virtual void on_sta_scan_done(wifi_ap_record_t *ap_list, uint16_t ap_num) { }

    virtual void on_sta_start_connect(uint8_t *ssid) { }
    virtual void on_sta_connect_success(wifi_event_sta_connected_t *event_data) { }
    virtual void on_sta_connect_failed(wifi_event_sta_disconnected_t *event_data) { }
    
    virtual void on_sta_connected(uint8_t *ssid, uint8_t *bssid) { }
    virtual void on_sta_disconnected(uint8_t *ssid, int32_t rssi, uint8_t reason) { }

    virtual void on_netcm_heart_handle() { }

private:

protected:

};


class Netcm {
public:
    static void netcm_init();

    /* station */
    static void sta_async_connect(const char *ssid, const char *password);
    static const char *sta_get_ssid();

    static bool sta_is_connected();
    static bool sta_is_connecting();

    static uint8_t *sta_get_mac_addr();
    static const char *sta_get_mac_str();
    static int32_t sta_get_rssi();

    /* ip */
    static bool ip_is_got();
    static const char *ip_get_local_addr_str();

    /* task */
    static void loop_start(int priority, int core_id);
    static void loop_forever(void *args);
    static void register_callback(NetcmCallback *callback);

    /* blufi */
    static esp_blufi_event_cb_t blufi_get_event_cb(void) { return blufi_event_callback; }

private:
    static void ip_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data);
    static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data);
    static void blufi_event_callback(esp_blufi_cb_event_t event, esp_blufi_cb_param_t *param);

protected:

};
