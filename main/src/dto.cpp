#include "dto.hpp"

#include "main.hpp"

#include "netcm.hpp"
#include "kxmqtt.hpp"
#include "fota.hpp"
#include "kxchat.hpp"
#include "kxchat/kxaudio.hpp"

#include "esp_log.h"


const char TAG[] = "kx_dto_utils";


char *DTO::ota_batch_id = NULL;


static mJSON create_msg_base(resp_code_t resp_code) {
    mJSON msg;

    msg.add("resp_code", resp_code);
    msg.add("battery", bsp_kxdemo.battery_get_level());
    msg.add("volume", KxAudio::kx_spk_get_volume());
    msg.add("mac_address", Netcm::sta_get_mac_str());
    msg.add("version", Fota::get_app_desc()->version);

    return msg;
}


void DTO::resp_recv_login_sucess(const char *msg_id) {
    mJSON msg = create_msg_base(RespLoginSuccessCode);
    msg.add("msg_id", msg_id);
    KxMQTT::publish_message(msg);
    msg.free();
}


void DTO::resp_recv_wifi_sucess(const char *msg_id) {
    mJSON msg = create_msg_base(RespWiFiSuccessCode);
    msg.add("msg_id", msg_id);
    KxMQTT::publish_message(msg);
    msg.free();
}


void DTO::ping() {
    mJSON msg = create_msg_base(ReportPingCode);
    KxMQTT::publish_message(msg);
    msg.free();
}


void DTO::resp_ota_sucess(const char *msg_id) {
    mJSON msg = create_msg_base(RespOtaSuccessCode);
    msg.add("msg_id", msg_id);
    if (NULL != ota_batch_id) {
        msg.add("batch_id", ota_batch_id);
    }
    KxMQTT::publish_message(msg);
    msg.free();
}


void DTO::resp_ota_fail(const char *msg_id) {
    mJSON msg = create_msg_base(RespOtaFailCode);
    msg.add("msg_id", msg_id);
    if (NULL != ota_batch_id) {
        msg.add("batch_id", ota_batch_id);
    }
    KxMQTT::publish_message(msg);
    msg.free();
}


void DTO::report_ota_progress(int progress) {
    mJSON msg = create_msg_base(ReportOtaProgressCode);
    msg.add("progress", progress);
    if (NULL != ota_batch_id) {
        msg.add("batch_id", ota_batch_id);
    }
    KxMQTT::publish_message(msg);
    msg.free();
}


void DTO::set_ota_batch_id(const char *batch_id) {
    if (NULL != batch_id) {
        if (NULL != ota_batch_id) {
            free(ota_batch_id);
            ota_batch_id = NULL;
        }
        ota_batch_id = strdup(batch_id);
    }
}
