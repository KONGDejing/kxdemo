#include "netcm_priv.hpp"

#include "esp_sntp.h"


static bool sntp_already_synced_flag = false;


static void time_update_callback(struct timeval* tv) 
{
    /* 设置时区 */
    setenv("TZ", "CST-8", 1);
    tzset();

    sntp_already_synced_flag = true;
}


void netcm_sntp_start() {
    // 检查 SNTP 功能是否已启用
    if (esp_sntp_enabled()) return;

    // 启用 SNTP 功能，并设置 NTP 服务器
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_setservername(1, "ntp.tencent.com");
    esp_sntp_setservername(2, "ntp.aliyun.com");
    esp_sntp_set_time_sync_notification_cb(time_update_callback);
    esp_sntp_init();
}


bool netcm_sntp_is_synced() {
    return sntp_already_synced_flag;
}


bool netcm_sntp_is_enabled() {
    return esp_sntp_enabled();
}
