#include "app_callbacks.hpp"

#include "main.hpp"
#include "dto.hpp"
#include "blink.hpp"

/* ESP IDF */
#include "esp_timer.h"

/* componetns */
#include "fota.hpp"
#include "netcm.hpp"
#include "kxchat.hpp"
#include "kxchat/kxaudio.hpp"
#include "kxfiles.h"


static const char TAG[] = "app_callbacks";

static void play_cached_mp3(kxfiles_file_id_t file_type)
{
    file_info_t file_info;
    esp_err_t ret = kxfiles_get_file_info(file_type, &file_info);
    if (ret == ESP_OK) {
        KxAudio::play_mp3_file((uint8_t*)file_info.start, file_info.size);
    } else {
        ESP_LOGE(TAG, "Failed to get  MP3 file (err=0x%X)", ret);
    }
}

void kx_chat_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    switch (event_id) {
        case KXCHAT_EVENT_RECORD_START:
            play_cached_mp3(KXFILES_BO_MP3);
        break;
        case KXCHAT_EVENT_RECORD_END:
            play_cached_mp3(KXFILES_XIU_MP3);
            break;
        case KXCHAT_EVENT_CHANGE_VOLUME:
            if (event_data != NULL) {
                int volume = *(int*)(event_data);
                KxAudio::kx_spk_set_volume(volume);
                ESP_LOGI(TAG, "Volume set to: %d", volume); 
            } else {
                ESP_LOGE(TAG, "Volume event data is NULL");
            }
        break;
        
        default:
            ESP_LOGW(TAG, "Unknown event id: %d", (int)event_id);
            break;
    }
}

void AppNetcmCallback::on_netif_got_ip(const char *ip) {
    KxMQTT::start();
    vTaskDelay(100 / portTICK_PERIOD_MS);
    play_cached_mp3(KXFILES_WIFI_CONNECT_SUCCESS_MP3);
}


void AppNetcmCallback::on_netif_lost_ip() {

}


void AppNetcmCallback::on_sta_start_scan(uint8_t *ssid) {

}


void AppNetcmCallback::on_sta_scan_done(wifi_ap_record_t *ap_list, uint16_t ap_num) {

}


void AppNetcmCallback::on_sta_start_connect(uint8_t *ssid) {
    play_cached_mp3(KXFILES_WIFI_CONNECTING_MP3);
}


void AppNetcmCallback::on_sta_connect_success(wifi_event_sta_connected_t *event_data) {
    play_cached_mp3(KXFILES_WIFI_CONNECT_SUCCESS_MP3);
}


void AppNetcmCallback::on_sta_connect_failed(wifi_event_sta_disconnected_t *event_data) {
    play_cached_mp3(KXFILES_WIFI_CONNECT_FAIL_MP3);
}


void AppNetcmCallback::on_sta_connected(uint8_t *ssid, uint8_t *bssid) {

}


void AppNetcmCallback::on_sta_disconnected(uint8_t *ssid, int32_t rssi, uint8_t reason) {
    KxMQTT::stop();
}


void AppNetcmCallback::on_netcm_heart_handle() {
    if (Netcm::ip_is_got() && Netcm::sta_is_connected()) {
        DTO::ping();
    } else {
        play_cached_mp3(KXFILES_NO_WIFI_MP3);
    }
}


/* play button event */
void button_play_event_cb(void *arg, void *data) {
    button_handle_t btn = (button_handle_t)arg;
    button_event_t event = iot_button_get_event(btn);
    iot_button_print_event(btn);

    switch (event) {
        case BUTTON_PRESS_DOWN: {
            if (Netcm::sta_is_connected() && Netcm::ip_is_got()) {
                if (KxAudio::play_is_playing()) {
                    KxAudio::play_stop();//停止播放音乐
                }
                if (!KxChat::is_recording()) {
                    KxChat::start_record();
                }
            } else {
                play_cached_mp3(KXFILES_NO_WIFI_MP3);
                ESP_LOGE(TAG, "Failed to play no WiFi MP3");
            }
            break;
        }

        case BUTTON_PRESS_UP: {
            if (Netcm::sta_is_connected() && Netcm::ip_is_got()) {
                KxChat::stop_record();
            }
            break;
        }

        case BUTTON_SINGLE_CLICK: {
            ESP_LOGI(TAG, "battery voltage: %.2fV", bsp_kxdemo.battery_get_voltage());
            ESP_LOGI(TAG, "battery level: %d%%", bsp_kxdemo.battery_get_level());
            break;
        }

        case BUTTON_DOUBLE_CLICK:
            // TODO: Implement double click action
            break;

        case BUTTON_LONG_PRESS_START:
            // TODO: Implement long press start action
            break;

        case BUTTON_LONG_PRESS_UP:
            // TODO: Implement long press up action
            break;

        default:
            ESP_LOGW(TAG, "Unhandled button event: %d", (int)event);
            break;
    }

}


void app_ota_progress_cb(void* arg, esp_event_base_t event_base,
                        int32_t event_id, void* event_data) {
    
    if (event_id == FOTA_EVENT_STARTED) {
        /* 推送 OTA 开始 */
        ESP_LOGI(TAG, "OTA开始");
        KxAudio::mic_pause_record();
        KxAudio::spk_stop_play();
        vTaskDelay(100 / portTICK_PERIOD_MS);
        play_cached_mp3(KXFILES_START_OTA_MP3);
    }
    else if (event_id == FOTA_EVENT_PROGRESS) {
        /* 推送 OTA 进度 */
        int progress = *(int*)event_data;
        DTO::report_ota_progress(progress);
    }
    else if (event_id == FOTA_EVENT_DONE) {
        play_cached_mp3(KXFILES_OTA_SUCCESS_MP3);
        KxMQTT::clear_broker_down_retain_message();
        ESP_LOGI(TAG, "OTA升级成功 重启设备");
    }
    else if (event_id == FOTA_EVENT_FAILED) {
        ESP_LOGI(TAG, "OTA失败 重启设备");
        play_cached_mp3(KXFILES_OTA_FAIL_MP3);
    }
}


/* mqtt callback */


void AppMqttCallback::on_connected() {
    // 连接服务器即发送一次PING消息
    DTO::ping();
}


void AppMqttCallback::on_disconnected() {

}


void AppMqttCallback::on_message(const char *data, int data_len) {
    ESP_LOGI(TAG, "Receive message %d: %s", data_len, data);
    
    mJSON msg = mJSON(data);
    
    do {
        /* 检查是否为json结构 */
        if (!msg.is_object()) {
            ESP_LOGI(TAG, "msg is not object");
            break;
        }

        req_data_t req_data = {
            .req_code = msg["req_code"],
            .user_id = msg["user_id"],
            .mac_address = msg["mac_address"],
            .data = msg["data"],
            .msg_id = msg["msg_id"],
        };

        /* 检查 req_code 数据类型 */
        if (!req_data.req_code.is_number()) {
            ESP_LOGI(TAG, "req_code is not number");
            break;
        }

        if (req_data.req_code.get_int() == ReqLoginCode) {
            ESP_LOGI(TAG, "获取登陆信息, Login success");  /* 登陆成功 */
            DTO::resp_recv_login_sucess(req_data.msg_id.get_string());
        }

        else if (req_data.req_code.get_int() == ReqWiFiCode) {
            ESP_LOGI(TAG, "获取wifi信息");
            /* 获取 ssid password */
            mJSON ssid = req_data.data["ssid"];
            mJSON password = req_data.data["password"];
            /* 校验 ssid password 类型 */
            if (ssid.is_string() && password.is_string()) {
                /* 校验密码长度 */
                if (strlen(password.get_string()) >= 8) {
                    ESP_LOGI(TAG, "ssid password check success");
                    DTO::resp_recv_wifi_sucess(req_data.msg_id.get_string());
                    Netcm::sta_async_connect(ssid.get_string(), password.get_string());
                }
            }
        }

        else if (req_data.req_code.get_int() == ReqOtaCode) {
            ESP_LOGI(TAG, "获取ota信息");
            mJSON url = req_data.data["url"];
            if (url.is_string()) {

                mJSON batch_id = msg["batch_id"];
                if (batch_id.is_string()) {
                    ESP_LOGI(TAG, "batch_id check success");
                    DTO::set_ota_batch_id(batch_id.get_string());
                }

                if (Fota::begin(url.get_string())) {
                    DTO::resp_ota_sucess(req_data.msg_id.get_string());
                } else {
                    DTO::resp_ota_fail(req_data.msg_id.get_string());
                }
            }
        }
    } while(0);

    msg.free();
}
