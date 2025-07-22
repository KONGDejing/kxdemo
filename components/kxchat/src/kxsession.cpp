#include "kxsession.hpp"
#include "kxchat.hpp"
#include "kxchat/kxopus.hpp"

#include "string.h"

#include "esp_timer.h"
#include "mbedtls/base64.h"

#include "esp_log.h"
#include "esp_event.h"
#include "kxchat.hpp"


#define EVT_RECV_DATA       (1<<0)
#define EVT_RECV_DONE       (1<<1)
#define EVT_RECV_ANY        (0xFFFF)

// esp_event_loop_handle_t kxchat_event_handle = NULL;
int volume_value = 0;
void KxSession::on_bytes_message(esp_websocket_event_data_t *event_data) {
    xEventGroupSetBits(event_group, EVT_RECV_DATA);

    if (event_data->payload_offset == 0) {
        binary_buffer = (char *)malloc(event_data->payload_len);
        memset(binary_buffer, 0x0, event_data->payload_len);
        memcpy(binary_buffer, event_data->data_ptr, event_data->data_len);
    }
    else {
        memcpy(binary_buffer + event_data->payload_offset, event_data->data_ptr, event_data->data_len);
    }

    if (event_data->data_len + event_data->payload_offset == event_data->payload_len) {
        kx_audio_frame_t frame = {NULL, 0, false};
        frame.data = binary_buffer;
        frame.data_size = event_data->payload_len;

        KxAudio::spk_push_voice(&frame);

        if (binary_buffer) {
            binary_buffer = NULL;
        }
    }
}


void KxSession::on_string_message(esp_websocket_event_data_t *event_data) {
    xEventGroupSetBits(event_group, EVT_RECV_DATA);

    mJSON msg = mJSON(event_data->data_ptr);
    if (msg.is_object()) {
        mJSON type = msg["type"];
        if (type.is_string()) {
            if (strstr(type.get_string(), "begin")) {
                KxAudio::spk_start_play();
                ESP_LOGI(get_name(), "开始会话");
            }
            else if (strstr(type.get_string(), "end"))
            {
                ESP_LOGI(get_name(), "结束会话");
                xEventGroupSetBits(event_group, EVT_RECV_DONE);
            }
            else if (strstr(type.get_string(), "audio_start"))
            {
                // ESP_LOGI(get_name(), "开始音频流");
                kx_audio_frame_t frame = {NULL, 0, false, KX_AUDIO_FRAME_CMD_RESUME};
                KxAudio::spk_push_voice(&frame);
            }
            else if (strstr(type.get_string(), "audio_end"))
            {
                // ESP_LOGI(get_name(), "结束音频流");
                kx_audio_frame_t frame = {NULL, 0, false, KX_AUDIO_FRAME_CMD_PAUSE};
                KxAudio::spk_push_voice(&frame);
            }
            else if (strstr(type.get_string(), "music"))
            {
                mJSON data = msg["data"];
                if (data.is_object()) {
                    mJSON audio_url = data["audio_url"];
                    if (audio_url.is_string()) {
                        KxAudio::play_push_mp3_url(audio_url.get_string());
                    }
                }
            }
            else if (strstr(type.get_string(), "poem"))
            {
                mJSON data = msg["data"];
                if (data.is_object()) {
                    mJSON audio_url = data["audio_url"];
                    if (audio_url.is_string()) {
                        KxAudio::play_push_mp3_url(audio_url.get_string());
                    }
                }
            }
        }
    }
    msg.free();

    //ESP_LOGI("CJSON","JSON String: %s\n", (char *)event_data->data_ptr);
    cJSON *root = cJSON_Parse(event_data->data_ptr);
    if (root == NULL) {
        return ;
    }
    cJSON *data = cJSON_GetObjectItemCaseSensitive(root, "data");
    // 2. 获取 "data" 对象
    if (data == NULL) {
        cJSON_Delete(root);
        return ;
    }
    // 3. 检查 "data" 是否为对象
    if (!cJSON_IsObject(data)) {
        cJSON_Delete(root);
        return ;
    }
    // 4. 获取 "volume" 值
    cJSON *volume = cJSON_GetObjectItemCaseSensitive(data, "volume");
    if (volume == NULL) {
        cJSON_Delete(root);
        return ;
    }
    // 5. 检查 "volume" 是否为数字
    if (!cJSON_IsNumber(volume)) {
        cJSON_Delete(root);
        return ;
    }
    // 6. 提取整数值
    volume_value = volume->valueint;

    ESP_LOGI("CJSON","Volume value: %d\n", volume_value);
    esp_event_post_to(
        KxChat::kxchat_event_handle,
        KXCHAT_EVENT,
        KXCHAT_EVENT_CHANGE_VOLUME,
        &volume_value,
        sizeof(volume_value),
        portMAX_DELAY
    );

    cJSON_Delete(root);
}


void KxSession::set_ws_url(const char *mac) {
    sprintf(ws_url, "ws://totoro-dev.kaixinyongyuan.cn:8096/algorithm/chat_opus?io_format=0&mac_address=%s", mac);
}


void KxSession::send_data_frame(kx_audio_frame_t *in) {
    mJSON msg;

    // 数据编码
    size_t slen = in->data_size;
    size_t dlen = 0;
    size_t olen = 0;

    if (slen % 3 == 0) {
        dlen = slen / 3 * 4 + 1;
    } else {
        dlen = (slen/3+1) * 4 + 1;
    }

    unsigned char *out = (unsigned char*)malloc(dlen * sizeof(char));
    if (NULL == out) {
        ESP_LOGI(get_name(), "not enough memory dlen %d olen %d", dlen, olen);
        return;
    }
    memset(out, 0x0, dlen * sizeof(char));

    int ret = mbedtls_base64_encode(out, dlen, &olen, (unsigned char*)(in->data), slen);
    if (ret != 0) {
        ESP_LOGI(get_name(), "encode fail %d dlen %d olen %d", ret, dlen, olen);
        return;
    }

    ESP_LOGI(get_name(), "dlen %d olen %d slen %d", dlen, olen, slen);

    // 发送数据
    msg.add("speech", (const char*)out);
    ws_send_text(msg, strlen(msg), 5000);

    // 释放内存
    msg.free();
    free(out);
}


void KxSession::send_opus_data_frame(kx_audio_frame_t *in) {
    int in_size = 0;
    int out_size = 0;
    uint8_t *out_data = NULL;

    KxOpusCodec::get_frame_size(&in_size, &out_size);
    ESP_LOGI(get_name(), "in_size %d out_size %d", in_size, out_size);

    out_data = (uint8_t*)malloc(out_size*sizeof(uint8_t));
    if (NULL == out_data) {
        ESP_LOGI(get_name(), "not enough memory in_size %d out_size %d", in_size, out_size);
        return;
    }
    memset(out_data, 0x0, out_size*sizeof(uint8_t));

    for (int i = 0; i < in->data_size; i += in_size) {
        
        esp_audio_enc_out_frame_t out;
        out.buffer = out_data;
        out.len = out_size;

        uint8_t *in_data = (uint8_t*)(in->data) + i;
        KxOpusCodec::encode(in_data, in_size, &out);

        ws_send_binary((const char*)out.buffer, out.encoded_bytes, 5000);
    }

    if (out_data) {
        free(out_data);
        out_data = NULL;
    }
}


void KxSession::send_end_frame() {
    mJSON msg;

    // 发送数据
    msg.add("speech", "end");
    ws_send_text(msg, strlen(msg), 5000);

    // 释放内存
    msg.free();

    xEventGroupClearBits(event_group, EVT_RECV_DATA);
    xEventGroupClearBits(event_group, EVT_RECV_DONE);
}


bool KxSession::is_recving() {
    EventBits_t event = xEventGroupWaitBits(
        event_group, 
        EVT_RECV_DATA,
        pdFALSE, 
        pdFALSE, 
        0
    );
    return (event & EVT_RECV_DATA) != 0;
}


bool KxSession::wait_recv_done(int timeout_ms) {
    EventBits_t event_bits;
    do {
        event_bits = xEventGroupWaitBits(
            event_group, 
            EVT_RECV_DATA | EVT_RECV_DONE,
            pdTRUE, 
            pdFALSE, 
            pdMS_TO_TICKS(timeout_ms)
        );

        if (event_bits & EVT_RECV_DONE) {
            return true;
        }
    } while ((event_bits & EVT_RECV_DATA));
    return false;
}
