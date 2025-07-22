
#pragma once

#ifndef __KX_AUDIO_HPP__
#define __KX_AUDIO_HPP__

#include <stdint.h>

#include "kxchat/kx_audio_types.hpp"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/idf_additions.h"

#include "esp_ns.h"
#include "esp_vad.h"
#include "esp_codec_dev.h"
#include "decoder/impl/esp_mp3_dec.h"

#include "esp_wn_iface.h"
#include "esp_wn_models.h"
#include "esp_afe_sr_iface.h"
#include "esp_afe_sr_models.h"
#include "esp_mn_iface.h"
#include "esp_mn_models.h"
#include "model_path.h"
#include "esp_mn_speech_commands.h"


class KxAudio {
public:
    static void kx_audio_init(esp_codec_dev_handle_t in_dev, esp_codec_dev_handle_t out_dev);
    static void kx_start_service(int priority, int core_id);

/* -------------------------------------------------- 音频播放相关 -------------------------------------------------- */
public:
    /**
     * @brief 设置音量
     * @param volume 音量 0-100
     */
    static void kx_spk_set_volume(int volume);

    /**
     * @brief 获取当前音量
     * @return 当前音量0-100
     */
    static int kx_spk_get_volume(void);

    /**
     * @brief 播放音频
     * @param data 音频数据
     * @param len 音频数据长度
     */
    static void spk_write(uint8_t *data, int len);

    /**
     * @brief 将音频数据写入到音频缓存队列中
     * @param frame 音频数据
     */
    static bool spk_push_voice(kx_audio_frame_t *frame);
    
    /**
     * @brief 获取音频缓存队列中音频数据数量
     */
    static int spk_get_buffer_count();

    /**
     * @brief 清空音频缓存队列
     */
    static void spk_clear_buffer();

    /**
     * @brief 开始播放音频缓存队列中的音频数据
     */
    static void spk_start_play();

    /**
     * @brief 停止播放
     */
    static void spk_stop_play();

    /**
     * @brief 等待音频缓存队列播放完成
     */
    static void spk_wait_play_done();

private:
    static esp_codec_dev_handle_t spk_codec_dev;

    static StackType_t *_spk_task_stack;
    static StaticTask_t _spk_task_tcb;
    static TaskHandle_t _spk_handle;

    static QueueHandle_t _spk_buffer;
    static SemaphoreHandle_t _spk_mutex;
    static bool _spk_running;

    static bool spk_pop_voice(kx_audio_frame_t *frame);
    static void open_spk_dev();
    static void spk_task(void *args);

/* -------------------------------------------------- 音频文件播放相关 -------------------------------------------------- */
public:
    static void play_init();
    static void play_stop();
    static bool play_is_playing();

    static void play_push_mp3_file(uint8_t *start, uint32_t size);
    static void play_push_mp3_url(const char *url);

    static void play_mp3_file(uint8_t *start, uint32_t size);
    static void play_mp3_url(const char *url);

private:
    static StackType_t *play_task_stack;
    static StaticTask_t play_task_tcb;
    static TaskHandle_t play_handle;
    static SemaphoreHandle_t play_mutex;
    static EventGroupHandle_t play_event_group;

    static void play_mp3_file_task(void *args);
    static void play_mp3_url_task(void *args);

/* -------------------------------------------------- 音频录制相关 -------------------------------------------------- */
public: // 音频录制相关
    /**
     * @brief 设置麦克风增益
     * @param gain 麦克风增益 0-100
     */
    static void kx_mic_set_gain(int gain);

    /**
     * @brief 开始录音
     */
    static void mic_start_record();

    /**
     * @brief 暂停录音
     */
    static void mic_pause_record();

    /**
     * @brief 是否正在录音
     * 
     * @return
     *      - true 正在录音
     *      - false 没有录音
     */
    static bool mic_is_recording();

    /**
     * @brief 获取麦克风缓存队列中音频数据
     * 
     * @param frame 输出音频
     */
    static bool mic_pop_voice(kx_audio_frame_t *frame);

    /**
     * @brief 获取麦克风缓存队列中音频数据数量
     */
    static int mic_get_buffer_count();

    /**
     * @brief 清空麦克风缓存队列
     */
    static void mic_clear_buffer();

private:
    static esp_codec_dev_handle_t mic_codec_dev;

    static TaskHandle_t _mic_handle;
    static QueueHandle_t _mic_buffer;
    static SemaphoreHandle_t _mic_mutex;
    static bool _mic_running;

    static bool mic_push_voice(kx_audio_frame_t *frame);
    static void open_mic_dev();
    static void mic_task(void *args);

public:
    /**
     * @brief 语音唤醒
     * 
     * @return
     *      - true 唤醒成功
     *      - false 唤醒失败
     */
    static bool wake_up();

private:
    static esp_afe_sr_iface_t *afe_handle;
    static esp_afe_sr_data_t *afe_data;
    static srmodel_list_t *models;

    static esp_mn_iface_t *multinet;
    static model_iface_data_t *model_data;

};


#endif /* __KX_AUDIO_HPP__ */
