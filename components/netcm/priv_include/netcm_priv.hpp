
#pragma once

#include "netcm.hpp"


#define TAG "netcm"


extern NetcmCallback *netcm_callback;
extern sta_auth_info_t *netcm_sta_pair_info;
extern sta_conn_result_t netcm_sta_conn_result;


#ifdef __cplusplus
extern "C" {
#endif


void netcm_sntp_start();
bool netcm_sntp_is_enabled();
bool netcm_sntp_is_synced();


/* 扫描 */
void netcm_sta_scan_all(bool block);
void netcm_sta_scan_stop();
void netcm_sta_scan_by_ssid(uint8_t *ssid);
void netcm_sta_scan_by_bssid(uint8_t *bssid);
wifi_ap_record_t *netcm_sta_scan_get_result(uint16_t *dst_ap_num);


#ifdef __cplusplus
}
#endif
