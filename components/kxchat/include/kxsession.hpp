
#pragma once

#ifndef __KX_SESSION_HPP__
#define __KX_SESSION_HPP__


#include "kxchat/kxaudio.hpp"
#include "kxchat/kx_audio_types.hpp"
#include "kxutils/websocket.hpp"
#include "kxutils/cxxjson.hpp"


// typedef enum {
//     KX_INTENT_OTHER = 0,
//     KX_INTENT_GREETING,                 // 打招呼
//     KX_INTENT_EMBRACE,                  // 拥抱
//     KX_INTENT_PRAISE_ME,                // 夸奖
//     KX_INTENT_SHAKE_HANDS,              // 握手
//     KX_INTENT_PLAY_MINDFULNESS_VIDEO,   // 播放正念冥想音频
//     KX_INTENT_STOP_PLAY_VIDEO,          // 停止播放音频
//     KX_INTENT_EXIT_CHAT,                // 退出会话
// }kx_session_intent_t;


class KxSession : public WebSocket {
public:
    using WebSocket::WebSocket;

    void set_ws_url(const char *mac);

    void send_data_frame(kx_audio_frame_t *in);
    void send_opus_data_frame(kx_audio_frame_t *in);
    void send_end_frame();

    bool is_recving();
    bool wait_recv_done(int timeout_ms);

    // const char *get_ws_url() { return "ws://192.168.137.1:8096/algorithm/chat_opus?io_format=0&device_id=1"; }
    // const char *get_ws_url() { return "ws://totoro-dev.kaixinyongyuan.cn:8096/algorithm/chat_opus?io_format=0&device_id=1"; }
    const char *get_ws_url() { return ws_url; }
    void on_bytes_message(esp_websocket_event_data_t *event_data);
    void on_string_message(esp_websocket_event_data_t *event_data);

private:
    char ws_url[128];

protected:

};


#endif /* __KX_SESSION_HPP__ */
