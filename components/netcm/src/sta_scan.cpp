#include "netcm_priv.hpp"

#include "esp_wifi.h"


void netcm_sta_scan_all(bool block) {
    wifi_scan_config_t scan_config = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .show_hidden = true,
        .scan_type = WIFI_SCAN_TYPE_ACTIVE,
        .scan_time = {
            .active = {
                .min = 0,
                .max = 0,
            }
        }
    };
    esp_wifi_scan_start(&scan_config, block);
}


void netcm_sta_scan_stop() {
    esp_wifi_scan_stop();
}


void netcm_sta_scan_by_ssid(uint8_t *ssid) {
    static uint8_t *target_ssid = ssid;

    /* 检查是否设置了 SSID */
    if (NULL != target_ssid) {
        if (strlen((const char*)ssid) == 0) {
            target_ssid = NULL;
        }
    }

    wifi_scan_config_t scan_config = {
        .ssid = target_ssid,
        .bssid = NULL,
        .channel = 0,
        .show_hidden = true,
        .scan_type = WIFI_SCAN_TYPE_ACTIVE,
        .scan_time = {
            .active = {
                .min = 0,
                .max = 0,
            }
        }
    };

    if (NULL != ssid) {
        ESP_LOGI(TAG, "wifi_scan_by_ssid %s", (char*)ssid);
    } else {
        ESP_LOGI(TAG, "wifi_scan all ssid");
    }
    
    esp_wifi_scan_start(&scan_config, true);
}


void netcm_sta_scan_by_bssid(uint8_t *bssid) {
    wifi_scan_config_t scan_config = {
        .ssid = NULL,
        .bssid = bssid,
        .channel = 0,
        .show_hidden = true,
        .scan_type = WIFI_SCAN_TYPE_ACTIVE,
        .scan_time = {
            .active = {
                .min = 0,
                .max = 0,
            }
        }
    };
    esp_wifi_scan_start(&scan_config, true);
}


wifi_ap_record_t *netcm_sta_scan_get_result(uint16_t *dst_ap_num) {
    uint16_t ap_num = 0;
    wifi_ap_record_t *ap_list = NULL;

    esp_wifi_scan_get_ap_num(&ap_num);

    if (ap_num > 0) {
        ap_list = (wifi_ap_record_t*)malloc(sizeof(wifi_ap_record_t) * ap_num);
        memset(ap_list, 0x0, sizeof(wifi_ap_record_t) * ap_num);
        esp_wifi_scan_get_ap_records(&ap_num, ap_list);
    }

    esp_wifi_scan_stop();

    *dst_ap_num = ap_num;
    return ap_list;
}
