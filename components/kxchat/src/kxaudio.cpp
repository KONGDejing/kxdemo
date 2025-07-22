#include "kxchat/kxaudio.hpp"
#include "kxchat/kxopus.hpp"

#include <string.h>
#include <stdio.h>

#include "esp_log.h"
#include "esp_timer.h"
#include "esp_http_client.h"


static const char TAG[] = "kx_audio";


// 沉默检测阈值
#define VAD_THRESHOLD       320
#define VAD_END_THRESHOLD   480
#define QUEUE_SIZE          (2000)
#define MIC_QUEUE_SIZE      (4)

#define KXSPK_TASK_STACK_SIZE    (40 * 1024)


esp_codec_dev_handle_t KxAudio::spk_codec_dev = NULL;
esp_codec_dev_handle_t KxAudio::mic_codec_dev = NULL;

StackType_t *KxAudio::_spk_task_stack = NULL;
StaticTask_t KxAudio::_spk_task_tcb; 
TaskHandle_t KxAudio::_spk_handle = NULL;

QueueHandle_t KxAudio::_spk_buffer = NULL;
SemaphoreHandle_t KxAudio::_spk_mutex = NULL;
bool KxAudio::_spk_running = false;

TaskHandle_t KxAudio::_mic_handle = NULL;
QueueHandle_t KxAudio::_mic_buffer = NULL;
SemaphoreHandle_t KxAudio::_mic_mutex = NULL;
bool KxAudio::_mic_running = NULL;

esp_afe_sr_iface_t *KxAudio::afe_handle = NULL;
esp_afe_sr_data_t *KxAudio::afe_data = NULL;
srmodel_list_t *KxAudio::models = NULL;

esp_mn_iface_t *KxAudio::multinet = NULL;
model_iface_data_t *KxAudio::model_data = NULL;


void KxAudio::spk_task(void *args) {
    kx_audio_frame_t frame = {NULL, 0, false};

    const int buffer_size = 1920;
    uint8_t *i2s_buffer = (uint8_t *)heap_caps_malloc(buffer_size, MALLOC_CAP_SPIRAM);

    int offset = 0;
    int write_len = 0;

    for (;;) {
        ESP_LOGI(TAG, "开始播放");

        while (_spk_running) {
            // 正在播放音乐 阻塞线程等待音乐播放完成
            if (play_is_playing()) {
                vTaskDelay(10 / portTICK_PERIOD_MS);
                continue;
            }

            if (spk_pop_voice(&frame)) {
                switch (frame.cmd)
                {
                    case KX_AUDIO_FRAME_CMD_PAUSE:
                        ESP_LOGI(TAG, "pause play");
                        break;
                
                    case KX_AUDIO_FRAME_CMD_RESUME:
                        ESP_LOGI(TAG, "resume play");
                        break;

                    case KX_AUDIO_FRAME_CMD_MP3_FILE:
                        ESP_LOGI(TAG, "start play mp3 from oss: %s", frame.data);
                        vTaskDelay(100 / portTICK_PERIOD_MS);
                        play_mp3_file((uint8_t*)frame.data, frame.data_size);
                        break;
                    
                    case KX_AUDIO_FRAME_CMD_MP3_URL:
                        ESP_LOGI(TAG, "start play mp3 from oss: %s", frame.data);
                        vTaskDelay(100 / portTICK_PERIOD_MS);
                        play_mp3_url(frame.data);
                        frame.free_data_memory();
                        break;

                    default:
                        break;
                }

                if (NULL != frame.data && frame.data_size > 0) {
                    // 解码
                    KxOpusCodec::decode((uint8_t *)frame.data, frame.data_size, (uint8_t *)i2s_buffer, buffer_size);

                    for (offset=0; offset < buffer_size;  ) {
                        write_len = 512;

                        if (write_len + offset > buffer_size) {
                            write_len = buffer_size - offset;
                        }

                        /* 数据写入 */
                        esp_codec_dev_write(spk_codec_dev, i2s_buffer + offset, write_len);

                        offset += write_len;
                    }

                    // 释放内存
                    frame.free_data_memory();
                }
            }
            else {
                vTaskDelay(10 / portTICK_PERIOD_MS);
            }
        }

        ESP_LOGI(TAG, "停止播放");
        vTaskSuspend(NULL);
    }
}


void KxAudio::mic_task(void *args) {
    // 计算 feed size
    int feed_chunksize = afe_handle->get_feed_chunksize(afe_data);
    int nch = afe_handle->get_feed_channel_num(afe_data);
    const int feed_size = feed_chunksize * nch * sizeof(int16_t);

    int16_t *i2s_buff = (int16_t*)heap_caps_malloc(feed_size, MALLOC_CAP_SPIRAM);
    assert(i2s_buff);

    ESP_LOGI(TAG, "afe feed chunksize %d channel %d feed_size %d",feed_chunksize, nch, feed_size);

    // 计算 frame_size
    int afe_chunksize = afe_handle->get_fetch_chunksize(afe_data);
    const int frame_size = afe_chunksize * sizeof(int16_t);
    const int frame_ms = afe_chunksize / 16;    // 32 ms
    const int frame_cnt = 15;   // 15 * 32 = 480 ms
    const int buffer_size = frame_cnt * frame_size;

    ESP_LOGI(TAG, "afe detect chunksize %d frame_size %d frame_ms %d ms frame_cnt %d", 
        afe_chunksize, frame_size, frame_ms, frame_cnt);

    kx_audio_frame_t frame = {NULL, 0};

    int vad_speech_ms = 0;
    int vad_silent_ms = 0;
    bool is_speaking = false;

    for (;;) {
        frame.data = NULL;
        frame.data_size = 0;

        /* 申请内存 */
        frame.data = (char*)heap_caps_malloc(buffer_size, MALLOC_CAP_SPIRAM);

        if (NULL == frame.data) {
            ESP_LOGE(TAG, "申请内存失败");
            vTaskDelay(100 / portTICK_PERIOD_MS);
            continue;
        }

        /* 读取数据 */
        for (int index=0; index < frame_cnt; ) {
            esp_codec_dev_read(mic_codec_dev, i2s_buff, feed_size);
            
            afe_handle->feed(afe_data, i2s_buff);
            afe_fetch_result_t* res = afe_handle->fetch(afe_data);

            if (res && res->ret_value != ESP_FAIL) {
                // 复制数据
                int offset = index * frame_size;
                memcpy(frame.data + offset, res->data, frame_size);
                frame.data_size += frame_size;
                index++;

                // vad数值计算
                if (res->vad_state) {
                    vad_speech_ms += frame_ms;
                    vad_silent_ms = 0;
                } else {
                    vad_silent_ms = vad_silent_ms > 480 ? 480 : vad_silent_ms + frame_ms;
                }

                if (vad_silent_ms >= 480) {
                    vad_speech_ms = 0;
                }
            }
        }

        if (!is_speaking) {
            if (vad_speech_ms >= VAD_THRESHOLD) {
                is_speaking = true;
            }
        }
        else if (vad_speech_ms == 0) {
            is_speaking = false;
        }

        frame.is_end = !is_speaking;

        /* 录音数据缓存 */
        mic_push_voice(&frame);
        ESP_LOGI(TAG, "vad_speech_ms %d vad_silent_ms %d data_size %d is_speaking %d", 
            vad_speech_ms, vad_silent_ms, frame.data_size, is_speaking);

        if (!_mic_running) {
            vad_speech_ms = 0;
            vad_silent_ms = 0;
            is_speaking = false;

            /* 清空缓存 */
            afe_handle->reset_buffer(afe_data);
            afe_handle->reset_vad(afe_data);

            vTaskSuspend(NULL); 
        }
    }

    if (i2s_buff) {
        free(i2s_buff);
    }

    vTaskDelete(NULL);
}


bool KxAudio::wake_up() {
    int audio_chunksize = afe_handle->get_feed_chunksize(afe_data);
    int nch = afe_handle->get_feed_channel_num(afe_data);
    const int feed_size = audio_chunksize * nch * sizeof(int16_t);

    int16_t *i2s_buff = (int16_t*)heap_caps_malloc(feed_size, MALLOC_CAP_SPIRAM);
    assert(i2s_buff);

    bool is_wakeup = false;

    do {
        esp_codec_dev_read(mic_codec_dev, i2s_buff, feed_size);

        afe_handle->feed(afe_data, i2s_buff);
        afe_fetch_result_t* res = afe_handle->fetch(afe_data);

        if (res && res->ret_value != ESP_FAIL) {
            // 命令词检测
            esp_mn_state_t mn_state = multinet->detect(model_data, res->data);

            if (mn_state == ESP_MN_STATE_DETECTING) {
                continue;
            }

            esp_mn_results_t *mn_result = multinet->get_results(model_data);
            if (strlen(mn_result->string) > 0) {
                ESP_LOGI(TAG, "识别结果: %d %s", mn_state, mn_result->string);
                int ret = 0;
                ret += (NULL != strstr(mn_result->string, "hai"));
                ret += (NULL != strstr(mn_result->string, "xiao"));
                ret += (NULL != strstr(mn_result->string, "yu"));
                ret += (NULL != strstr(mn_result->string, "tong"));
                ret += (NULL != strstr(mn_result->string, "xue"));
                if (ret > 0) {
                    ESP_LOGI(TAG, "唤醒成功");
                    is_wakeup = true;
                    break;
                } 
                else {
                    ESP_LOGI(TAG, "唤醒失败");
                    break;
                }
            } else {
                ESP_LOGI(TAG, "未检测到关键词");
                break;
            }
        } 
        else {
            ESP_LOGI(TAG, "fetch failed %d", res->ret_value);
            break;
        }
    } while(1);

    /* 清空缓存 */
    afe_handle->reset_buffer(afe_data);
    afe_handle->reset_vad(afe_data);

    if (i2s_buff) {
        free(i2s_buff);
    }

    return is_wakeup;
}


/* extern function */


void KxAudio::kx_mic_set_gain(int gain) {
    /* Microphone input gain */
    esp_codec_dev_set_in_gain(mic_codec_dev, gain);
}


void KxAudio::kx_spk_set_volume(int volume) {
    /* Speaker output gain */
    esp_codec_dev_set_out_vol(spk_codec_dev, volume);
}


int KxAudio::kx_spk_get_volume(void) {
    int volume = 0;
    esp_codec_dev_get_out_vol(spk_codec_dev, &volume);
    return volume;
}


void KxAudio::open_mic_dev() {
    esp_codec_dev_sample_info_t fs = {
        .bits_per_sample = FRAME_BITS,
        .channel = 2,
        // .channel_mask = 1,
        .sample_rate = SAMPLE_RATE_HZ,
    };
    esp_codec_dev_open(mic_codec_dev, &fs);
}


void KxAudio::open_spk_dev() {
    esp_codec_dev_sample_info_t fs = {
        .bits_per_sample = FRAME_BITS,
        .channel = 1,
        // .channel_mask = 1,
        .sample_rate = SAMPLE_RATE_HZ,
    };
    esp_codec_dev_open(spk_codec_dev, &fs);
}


void KxAudio::kx_audio_init(esp_codec_dev_handle_t in_dev, esp_codec_dev_handle_t out_dev) {
    mic_codec_dev = in_dev;
    assert(mic_codec_dev);

    spk_codec_dev = out_dev;
    assert(spk_codec_dev);

    ESP_LOGI(TAG, "open mic dev");
    open_mic_dev();
    ESP_LOGI(TAG, "open spk dev");
    if (in_dev != out_dev) {
        open_spk_dev();
    }

    /* AFE Initialization */
    models = esp_srmodel_init("model");
    afe_config_t *afe_config = afe_config_init("MR", models, AFE_TYPE_SR, AFE_MODE_HIGH_PERF);
    afe_handle = esp_afe_handle_from_config(afe_config);
    afe_data = afe_handle->create_from_config(afe_config);
    afe_config_free(afe_config);

    int audio_chunksize = afe_handle->get_feed_chunksize(afe_data);
    int nch = afe_handle->get_feed_channel_num(afe_data);
    ESP_LOGI(TAG, "audio_chunksize %d nch %d", audio_chunksize, nch);

    /* 命令词初始化 */
    char *mn_name = esp_srmodel_filter(models, ESP_MN_PREFIX, ESP_MN_CHINESE);
    multinet = esp_mn_handle_from_name(mn_name);
    model_data = multinet->create(mn_name, 3000);  // The duration (ms) to trigger the timeout
    int mu_chunksize = multinet->get_samp_chunksize(model_data);

    ESP_LOGI(TAG, "mn_name %s mu_chunksize %d", mn_name, mu_chunksize);

    esp_mn_commands_clear();
    esp_mn_commands_add(1, "hai xiao yu tong xue");
    esp_mn_commands_add(2, "hai xiao yu tong");
    esp_mn_commands_add(3, "hai xiao yu");
    esp_mn_commands_add(4, "hai xiao");
    esp_mn_commands_add(5, "xiao yu tong xue");
    esp_mn_commands_add(6, "xiao yu tong");
    esp_mn_commands_add(7, "yu tong xue");
    esp_mn_commands_add(8, "xiao yu");
    esp_mn_commands_add(9, "tong xue");
    esp_mn_commands_update();
    esp_mn_active_commands_print();

    /* opus codec */
    KxOpusCodec::init();

    _spk_buffer = xQueueCreate(QUEUE_SIZE, sizeof(kx_audio_frame_t));
    _spk_mutex = xSemaphoreCreateMutex();

    _mic_buffer = xQueueCreate(MIC_QUEUE_SIZE, sizeof(kx_audio_frame_t));
    _mic_mutex = xSemaphoreCreateMutex();
}


void KxAudio::kx_start_service(int priority, int core_id) {
    _spk_task_stack = (StackType_t *)malloc(KXSPK_TASK_STACK_SIZE * sizeof(StackType_t));
    memset(_spk_task_stack, 0x0, KXSPK_TASK_STACK_SIZE * sizeof(StackType_t));
    _spk_handle = xTaskCreateStaticPinnedToCore(
        spk_task,
        "spk_task",
        KXSPK_TASK_STACK_SIZE,
        NULL,
        (UBaseType_t)(priority),
        _spk_task_stack,
        &_spk_task_tcb,
        core_id
    );

    xTaskCreatePinnedToCore(mic_task, "mic_task", 4096, NULL, (UBaseType_t)(priority+1), &_mic_handle, core_id);
}


/* play */

void KxAudio::spk_write(uint8_t *data, int len) {
    if (NULL == spk_codec_dev) return;
    esp_codec_dev_write(spk_codec_dev, data, len);
}


bool KxAudio::spk_push_voice(kx_audio_frame_t *frame) {
    if (NULL == frame) { return false; }
    
    if (xSemaphoreTake(_spk_mutex, portMAX_DELAY) == pdTRUE) {
        if (uxQueueSpacesAvailable(_spk_buffer) == 0) {
            kx_audio_frame_t temp = {NULL, 0, false};
            xQueueReceive(_spk_buffer, &temp, 0);
            temp.free_data_memory();
        }
        xQueueSend(_spk_buffer, frame, 0);
        xSemaphoreGive(_spk_mutex);
        return true;
    }
    return false;
}


bool KxAudio::spk_pop_voice(kx_audio_frame_t *frame) {
    kx_audio_frame_t temp = {NULL, 0, false};
    if (xSemaphoreTake(_spk_mutex, portMAX_DELAY) == pdTRUE) {
        if (uxQueueMessagesWaiting(_spk_buffer) > 0) {
            xQueueReceive(_spk_buffer, &temp, portMAX_DELAY);
        }
        xSemaphoreGive(_spk_mutex);
    }
    frame->data = temp.data;
    frame->data_size = temp.data_size;
    frame->is_end = temp.is_end;
    frame->cmd = temp.cmd;
    return (NULL != frame->data);
}


int KxAudio::spk_get_buffer_count() {
    return uxQueueMessagesWaiting(_spk_buffer);
}


void KxAudio::spk_clear_buffer() {
    if (xSemaphoreTake(_spk_mutex, portMAX_DELAY) == pdTRUE) {
        kx_audio_frame_t frame;
        while (uxQueueMessagesWaiting(_spk_buffer) > 0) {
            xQueueReceive(_spk_buffer, &frame, portMAX_DELAY);
            frame.free_data_memory();
        }
        xSemaphoreGive(_spk_mutex);
    }
}


void KxAudio::spk_start_play() {
    if (NULL == _spk_handle) return;

    _spk_running = true;
    if (eTaskGetState(_spk_handle) == eSuspended) {
        vTaskResume(_spk_handle);
    }
}


void KxAudio::spk_stop_play() {
    _spk_running = false;
}


void KxAudio::spk_wait_play_done() {
    while (spk_get_buffer_count() > 0)
    {
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}


/* record */


void KxAudio::mic_start_record() {
    if (mic_is_recording()) return;

    _mic_running = true;
    vTaskResume(_mic_handle);
}


void KxAudio::mic_pause_record() {
    _mic_running = false;
    while (mic_is_recording()) {
        vTaskDelay(100);
    }
}


bool KxAudio::mic_is_recording() {
    if (NULL == _mic_handle) return false;
    return (eTaskGetState(_mic_handle) != eSuspended);
}


bool KxAudio::mic_push_voice(kx_audio_frame_t *frame) {
    if (NULL == frame) { return false; }
    
    if (xSemaphoreTake(_mic_mutex, portMAX_DELAY) == pdTRUE) {
        if (uxQueueSpacesAvailable(_mic_buffer) == 0) {
            kx_audio_frame_t temp = {NULL, 0, false};
            xQueueReceive(_mic_buffer, &temp, 0);
            temp.free_data_memory();
        }
        xQueueSend(_mic_buffer, frame, 0);
        xSemaphoreGive(_mic_mutex);
        return true;
    }
    return false;
}


bool KxAudio::mic_pop_voice(kx_audio_frame_t *frame) {
    kx_audio_frame_t temp = {NULL, 0, false};
    if (xSemaphoreTake(_mic_mutex, portMAX_DELAY) == pdTRUE) {
        if (uxQueueMessagesWaiting(_mic_buffer) > 0) {
            xQueueReceive(_mic_buffer, &temp, portMAX_DELAY);
        }
        xSemaphoreGive(_mic_mutex);
    }
    frame->data = temp.data;
    frame->data_size = temp.data_size;
    frame->is_end = temp.is_end;
    return (NULL != frame->data);
}


int KxAudio::mic_get_buffer_count() {
    return uxQueueMessagesWaiting(_mic_buffer);
}


void KxAudio::mic_clear_buffer() {
    if (xSemaphoreTake(_mic_mutex, portMAX_DELAY) == pdTRUE) {
        kx_audio_frame_t frame;
        while (uxQueueMessagesWaiting(_mic_buffer) > 0) {
            xQueueReceive(_mic_buffer, &frame, portMAX_DELAY);
            frame.free_data_memory();
        }
        xSemaphoreGive(_mic_mutex);
    }
}


/* -------------------------------------------------- 音频文件播放相关 -------------------------------------------------- */

StackType_t *KxAudio::play_task_stack = NULL;
StaticTask_t KxAudio::play_task_tcb;
TaskHandle_t KxAudio::play_handle = NULL;
SemaphoreHandle_t KxAudio::play_mutex = NULL;
EventGroupHandle_t KxAudio::play_event_group;
static char oss_url[1024] = {0};

#define PLAY_TASK_STACK_SIZE (1024 * 500)
#define PLAY_EVENT_STOP     (BIT0)


void KxAudio::play_init() {
    play_mutex = xSemaphoreCreateMutex();
    play_event_group = xEventGroupCreate();
    play_task_stack = (StackType_t *)malloc(PLAY_TASK_STACK_SIZE);
    memset(play_task_stack, 0x0, PLAY_TASK_STACK_SIZE);
}


void KxAudio::play_stop() {
    xEventGroupSetBits(play_event_group, PLAY_EVENT_STOP);
}


bool KxAudio::play_is_playing() {
    if (NULL != play_handle) {
        return (eTaskGetState(play_handle) != eDeleted);
    }
    return false;
}


void KxAudio::play_push_mp3_file(uint8_t *start, uint32_t size) {
    kx_audio_frame_t frame = {NULL, 0, false, KX_AUDIO_FRAME_CMD_MP3_FILE};
    frame.data_size = size;
    frame.data = (char*)start;
    spk_push_voice(&frame);
}


void KxAudio::play_push_mp3_url(const char *url) {
    kx_audio_frame_t frame = {NULL, 0, false, KX_AUDIO_FRAME_CMD_MP3_URL};
    frame.data_size = strlen(url) + 1;
    frame.data = (char*) malloc(frame.data_size);
    memcpy(frame.data, url, frame.data_size);
    spk_push_voice(&frame);
}


void KxAudio::play_mp3_file(uint8_t *start, uint32_t size) {
    if (xSemaphoreTake(play_mutex, 100 / portTICK_PERIOD_MS) == pdTRUE) {
        if (play_handle != NULL) {
            if (eTaskGetState(play_handle) == eDeleted) {
                play_handle = NULL;
                bzero(play_task_stack, PLAY_TASK_STACK_SIZE);
                bzero(&play_task_tcb, sizeof(StaticTask_t));
            }
        }

        if (play_handle == NULL) {
            kx_audio_frame_t in_raw = {
                .data = (char*)start,
                .data_size = (int)size
            };

            play_handle = xTaskCreateStatic(
                play_mp3_file_task, 
                "play_task", 
                PLAY_TASK_STACK_SIZE, 
                &in_raw, 
                6, 
                play_task_stack, 
                &play_task_tcb
            );
            ESP_LOGI(TAG, "play_mp3_file start");
        }
        else {
            ESP_LOGI(TAG, "play_task is running");
        }
        
        xSemaphoreGive(play_mutex);
    }
    else {
        ESP_LOGW(TAG, "play_mutex is not available");
    }
}


void KxAudio::play_mp3_url(const char *url) {
    bzero(oss_url, 1024);
    sprintf(oss_url, "http%s", url+5);   // https -> http

    if (xSemaphoreTake(play_mutex, 100 / portTICK_PERIOD_MS) == pdTRUE) {
        if (play_handle != NULL) {
            if (eTaskGetState(play_handle) == eDeleted) {
                play_handle = NULL;
                bzero(play_task_stack, PLAY_TASK_STACK_SIZE);
                bzero(&play_task_tcb, sizeof(StaticTask_t));
            }
        }

        if (play_handle == NULL) {
            play_handle = xTaskCreateStatic(
                play_mp3_url_task, 
                "play_task", 
                PLAY_TASK_STACK_SIZE, 
                (void *)NULL, 
                6, 
                play_task_stack, 
                &play_task_tcb
            );
            ESP_LOGI(TAG, "play_mp3_from_oss start");
        }
        else {
            ESP_LOGI(TAG, "play_task is running");
        }
        
        xSemaphoreGive(play_mutex);
    }
    else {
        ESP_LOGW(TAG, "play_mutex is not available");
    }
}


void KxAudio::play_mp3_file_task(void *arg) {
    EventBits_t event;

    esp_audio_dec_handle_t dec_handle = NULL;

    esp_audio_dec_in_raw_t raw;
    esp_audio_dec_out_frame_t out_frame;
    esp_audio_dec_info_t info;

    raw.buffer = (uint8_t*)(((kx_audio_frame_t *)arg)->data);
    raw.len = (uint32_t)(((kx_audio_frame_t *)arg)->data_size);

    out_frame.buffer = (uint8_t *) malloc(4096);
    memset(out_frame.buffer, 0x0, 4096);
    out_frame.len = 4096;

    esp_err_t ret = ESP_AUDIO_ERR_OK;
    int offset = 0;
    int write_len = 0;

    esp_mp3_dec_open(NULL, 0, &dec_handle);

    while (raw.len)
    {
        ret = esp_mp3_dec_decode(dec_handle, &raw, &out_frame, &info);
        
        if (ret == ESP_AUDIO_ERR_OK) {
            if (out_frame.decoded_size > 0) {
                for (offset = 0; offset < out_frame.decoded_size; ) {
                    write_len = 512;

                    if (write_len + offset > out_frame.decoded_size) {
                        write_len = out_frame.decoded_size - offset;
                    }
                    spk_write(out_frame.buffer + offset, write_len);

                    offset += write_len;
                }
            }

            raw.buffer += raw.consumed;
            raw.len -= raw.consumed;
        }

        else if (ret == ESP_AUDIO_ERR_BUFF_NOT_ENOUGH) {
            uint8_t *new_frame_data = (uint8_t*) realloc(out_frame.buffer, out_frame.needed_size);
            if (new_frame_data == NULL) {
                break;
            }
            out_frame.buffer = new_frame_data;
            out_frame.len = out_frame.needed_size;
        }

        else {
            break;
        }

        event = xEventGroupWaitBits(play_event_group, PLAY_EVENT_STOP, pdFALSE, pdFALSE, 0);
        if (event & PLAY_EVENT_STOP) {
            xEventGroupClearBits(play_event_group, PLAY_EVENT_STOP);
            break;
        }
    }

    if (out_frame.buffer) {
        free(out_frame.buffer);
    }

    esp_mp3_dec_close(dec_handle);

    ESP_LOGI(TAG, "play_mp3_file_task exit");
    
    vTaskDelete(NULL);
}


void KxAudio::play_mp3_url_task(void *arg) {
    #define OSS_BUFFER_SIZE (1 * 1024)

    EventBits_t event;
    esp_audio_dec_handle_t dec_handle = NULL;

    esp_audio_dec_in_raw_t raw;
    esp_audio_dec_out_frame_t out_frame;
    esp_audio_dec_info_t info;

    int read_rety = 0;
    int read_len = 0;
    int total_read_len = 0;

    int raw_offset = 0;
    raw.consumed = 0;
    uint8_t *temp_buffer1 = (uint8_t *) malloc(OSS_BUFFER_SIZE);
    memset(temp_buffer1, 0x0, OSS_BUFFER_SIZE);
    uint8_t *temp_buffer2 = (uint8_t *) malloc(OSS_BUFFER_SIZE);
    memset(temp_buffer2, 0x0, OSS_BUFFER_SIZE);

    out_frame.buffer = (uint8_t *) malloc(OSS_BUFFER_SIZE);
    memset(out_frame.buffer, 0x0, OSS_BUFFER_SIZE);
    out_frame.len = OSS_BUFFER_SIZE;

    esp_err_t dec_ret = ESP_AUDIO_ERR_OK;
    int offset = 0;
    int write_len = 0;

    ///
    esp_http_client_config_t cfg = {
        .url = (const char*) oss_url,
        .timeout_ms = 10000,
        // .buffer_size = OSS_BUFFER_SIZE,
    };

    ///
    esp_http_client_handle_t client = esp_http_client_init(&cfg);

    esp_mp3_dec_open(NULL, 0, &dec_handle);

    ESP_LOGI(TAG, "url: %s", oss_url);
    esp_err_t err = esp_http_client_open(client, 0);

    int content_length =  esp_http_client_fetch_headers(client);

    if (content_length > 0) {
        while (!esp_http_client_is_complete_data_received(client)) {
            // 读数据
            read_len = esp_http_client_read(client, (char*)(temp_buffer1 + raw_offset), OSS_BUFFER_SIZE - raw_offset);
            ESP_LOGI(TAG, "esp_http_client_read %d bytes", read_len);

            if (read_len == 0) {
                ESP_LOGW(TAG, "read empty data, wait 100 ms and rety %d...", read_rety);
                vTaskDelay(100 / portTICK_PERIOD_MS);
                
                read_rety++;
                if (read_rety > 10) {
                    break;
                }

                continue;
            }

            else if (read_len < 0) {
                ESP_LOGE(TAG, "esp_http_client_read failed");
                break;
            }

            total_read_len += read_len;
            ESP_LOGI(TAG, "total_read_len: %d / %d", total_read_len, content_length);

            raw.buffer = temp_buffer1;
            raw.len = read_len + raw_offset;

            // 解码 播放
            while (raw.len)
            {
                dec_ret = esp_mp3_dec_decode(dec_handle, &raw, &out_frame, &info);
                
                if (dec_ret == ESP_AUDIO_ERR_OK) {
                    if (out_frame.decoded_size > 0) {
                        for (offset = 0; offset < out_frame.decoded_size; ) {
                            write_len = 512;

                            if (write_len + offset > out_frame.decoded_size) {
                                write_len = out_frame.decoded_size - offset;
                            }
                            spk_write(out_frame.buffer + offset, write_len);

                            offset += write_len;
                        }
                    }

                    raw.buffer += raw.consumed;
                    raw.len -= raw.consumed;
                }

                else if (dec_ret == ESP_AUDIO_ERR_BUFF_NOT_ENOUGH) {
                    uint8_t *new_frame_data = (uint8_t*) realloc(out_frame.buffer, out_frame.needed_size);
                    if (new_frame_data == NULL) {
                        break;
                    }
                    out_frame.buffer = new_frame_data;
                    out_frame.len = out_frame.needed_size;
                }

                else {
                    if (raw.len > 0) {
                        raw_offset = raw.len;
                        memcpy(temp_buffer2, raw.buffer, raw.len);
                        memcpy(temp_buffer1, temp_buffer2, raw_offset);
                    }
                    break;
                }
            }

            event = xEventGroupWaitBits(play_event_group, PLAY_EVENT_STOP, pdFALSE, pdFALSE, 0);
            if (event & PLAY_EVENT_STOP) {
                xEventGroupClearBits(play_event_group, PLAY_EVENT_STOP);
                break;
            }
        }
        ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %"PRId64,
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
    }

    esp_http_client_close(client);
    esp_http_client_cleanup(client);

    if (temp_buffer1) {
        free(temp_buffer1);
    }

    if (temp_buffer2) {
        free(temp_buffer2);
    }

    if (out_frame.buffer) {
        free(out_frame.buffer);
    }

    esp_mp3_dec_close(dec_handle);

    ESP_LOGI(TAG, "play_mp3_url_task exit");

    vTaskDelete(NULL);
}
