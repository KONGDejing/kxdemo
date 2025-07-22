#include "kxchat.hpp"
/* esp */
#include "esp_log.h"
#include "esp_err.h"
#include "esp_mac.h"
/* kxchat */
#include "kxchat/kxaudio.hpp"
#include "kxsession.hpp"
/* others */
#include "kxutils/nvs_store.hpp"


static const char TAG[] = "kx_chat";

ESP_EVENT_DEFINE_BASE(KXCHAT_EVENT);

// #define CHAT_EVENT_PAUSE            (BIT0)
// #define CHAT_EVENT_EXIT             (BIT1)
// #define CHAT_EVENT_WAKEUP_ENABLE    (BIT2)
// #define CHAT_EVENT_WAKEUP           (BIT3)
#define CHAT_EVENT_ALL                  (0xFFFF)

#define CHAT_EVENT_RECORD_START         (BIT0)
#define CHAT_EVENT_RECORDING            (BIT1)
#define CHAT_EVENT_RECORD_END           (BIT2)
#define CHAT_EVENT_SPEAKING             (BIT3)

#define KXCHAT_TASK_CORE    0
#define RECV_TIMEOUT_MS    (20 * 1000)

#define KXCHAT_TASK_STACK_SIZE    (50 * 1024)

esp_event_loop_handle_t KxChat::kxchat_event_handle = NULL;

StackType_t *KxChat::chat_task_stack = NULL;
StaticTask_t KxChat::chat_task_tcb;
TaskHandle_t KxChat::chat_task_handle = NULL;
EventGroupHandle_t KxChat::event_group = NULL;


void KxChat::init() {
    /* 创建事件组 */
    event_group = xEventGroupCreate();
}


void KxChat::start_chat_task(int priority, int core_id) {
    /* 初始化事件循环 */
    esp_event_loop_args_t args = {
        .queue_size = 10,
        .task_name = "kx_chat_task",
        .task_priority = (UBaseType_t)(priority + 1),
        .task_stack_size = 4096,
        .task_core_id = core_id
    };
    
    esp_err_t err = esp_event_loop_create(&args, &kxchat_event_handle);
    if (err!= ESP_OK) {
        ESP_LOGE(TAG, "create event loop failed, error: %s", esp_err_to_name(err));
        return;
    }

    // esp_event_loop_args_t volume_args = {
    //     .queue_size = 10,
    //     .task_name = "kx_change_volume_task",
    //     .task_priority = (UBaseType_t)(priority + 1),
    //     .task_stack_size = 4096,
    //     .task_core_id = core_id
    // };
    
    // esp_err_t volume_err = esp_event_loop_create(&volume_args, &Kx_set_volume_event_loop_handle);
    //ESP_LOGI("KX volume", " Create  Kx_set_volume_event_loop_handle");
    // if (volume_err!= ESP_OK) {
    //     ESP_LOGE(TAG, "create Kx_set_volume_event_loop_handle failed, error: %s", esp_err_to_name(volume_err));
    //     return;
    // }

    /* 创建聊天任务 */
    chat_task_stack = (StackType_t *)malloc(KXCHAT_TASK_STACK_SIZE * sizeof(StackType_t));
    memset(chat_task_stack, 0x0, KXCHAT_TASK_STACK_SIZE * sizeof(StackType_t));
    chat_task_handle = xTaskCreateStaticPinnedToCore(
        chat_task,
        "chat_task",
        KXCHAT_TASK_STACK_SIZE,
        NULL,
        priority,
        chat_task_stack,
        &chat_task_tcb,
        core_id
    );
}


// void KxChat::pause_chat_task() {
//     exit_chat();
//     xEventGroupSetBits(event_group, CHAT_EVENT_PAUSE);
// }


// void KxChat::resume_chat_task() {
//     if (NULL == chat_task_handle) {
        
//     } else {
//         if (is_chat_paused()) {
//             ESP_LOGI(TAG, "resume chat task");
//             xEventGroupClearBits(event_group, CHAT_EVENT_PAUSE);
//             vTaskResume(chat_task_handle);
//         }
//     }
// }


// bool KxChat::is_chat_paused() {
//     return eTaskGetState(chat_task_handle) == eSuspended;
// }


void KxChat::start_record(void) {
    xEventGroupSetBits(event_group, CHAT_EVENT_RECORD_START);
}


void KxChat::stop_record(void) {
    xEventGroupSetBits(event_group, CHAT_EVENT_RECORD_END);
}


bool KxChat::is_recording() {
    EventBits_t event;
    event = xEventGroupWaitBits(
        event_group,
        CHAT_EVENT_RECORDING,
        pdFALSE,
        pdFALSE,
        0
    );
    return (event & CHAT_EVENT_RECORDING) != 0;
}


bool KxChat::is_speaking(void) {
    EventBits_t event;
    event = xEventGroupWaitBits(
        event_group,
        CHAT_EVENT_SPEAKING,
        pdFALSE,
        pdFALSE,
        0
    );
    return (event & CHAT_EVENT_SPEAKING) != 0;
}


// void KxChat::start_chat(void) {
//     xEventGroupSetBits(event_group, CHAT_EVENT_WAKEUP_ENABLE);
// }


// void KxChat::exit_chat(void) {
//     xEventGroupSetBits(event_group, CHAT_EVENT_EXIT);
// }


// bool KxChat::is_wake_up(void) {
//     EventBits_t event;
//     event = xEventGroupWaitBits(
//         event_group,
//         CHAT_EVENT_WAKEUP,
//         pdFALSE,
//         pdFALSE,
//         0
//     );
//     return (event & CHAT_EVENT_WAKEUP) != 0;
// }


void KxChat::spk_wait_play_done() {
    /* 等待语音播放完成 */
    KxAudio::spk_wait_play_done();
    /* 停止播放 */
    KxAudio::spk_stop_play();
}


// void KxChat::chat_task(void *arg) {
//     KxSession kx_session = KxSession("kx_session");

//     kx_audio_frame_t input_frame = {NULL, 0, false};
//     kx_audio_frame_t *output_frame = NULL;
//     kx_audio_frame_ring_queue_t ring_buffer = kx_audio_frame_ring_queue(4);

//     bool is_speaking = false;
//     EventBits_t event;

//     /* 延迟启动 */
//     vTaskDelay(1000 / portTICK_PERIOD_MS);

//     for (;;) {
//         event = xEventGroupWaitBits(
//             event_group,
//             CHAT_EVENT_PAUSE | CHAT_EVENT_WAKEUP,
//             pdFALSE,
//             pdFALSE,
//             0
//         );

//         if (event & CHAT_EVENT_PAUSE) {
//             ESP_LOGI(TAG, "chat paused");
//             xEventGroupClearBits(event_group, CHAT_EVENT_PAUSE);
//             /* 任务挂起 */
//             vTaskSuspend(NULL);
//         }

//         else if ((event & CHAT_EVENT_WAKEUP_ENABLE) == 0) {
//             if (KxAudio::wake_up()) {
//                 ESP_LOGI(TAG, "chat wakeup");
//                 xEventGroupSetBits(event_group, CHAT_EVENT_WAKEUP_ENABLE);
//             }
//         }

//         /* 等待唤醒 */
//         if (event & CHAT_EVENT_WAKEUP_ENABLE) {
//             xEventGroupClearBits(event_group, CHAT_EVENT_WAKEUP_ENABLE);
//             xEventGroupSetBits(event_group, CHAT_EVENT_WAKEUP);
//             ESP_LOGI(TAG, "wake up");

//             /* 开始对话 */
//             kx_session.new_session();
//             is_speaking = false;

//             /* 开始录音 */
//             KxAudio::mic_clear_buffer();
//             KxAudio::mic_start_record();

//             ESP_LOGI(TAG, "开始对话");

//             for (;;) {
//                 if (KxAudio::mic_pop_voice(&input_frame)) {
//                     if (ring_buffer.is_full()) {
//                         ring_buffer.overwrite(&input_frame);
//                     } else {
//                         ring_buffer.push(&input_frame);
//                     }

//                     if (!is_speaking) {
//                         if (!input_frame.is_end) {
//                             is_speaking = true;

//                             kx_session.new_session();
//                             ESP_LOGI(TAG, "说话中...");
//                         }
//                     }

//                     if (is_speaking) {
//                         output_frame = ring_buffer.pop();
//                         if (NULL != output_frame) {
//                             // kx_session.send_data_frame(output_frame);
//                             kx_session.send_opus_data_frame(output_frame);
//                             output_frame->free_data_memory();
//                             output_frame = NULL;
//                         }

//                         if (input_frame.is_end) {
//                             is_speaking = false;
                            
//                             /* 暂停录音 */
//                             KxAudio::mic_pause_record();

//                             while (!ring_buffer.is_empty())
//                             {
//                                 output_frame = ring_buffer.pop();
//                                 if (NULL != output_frame) {
//                                     // kx_session.send_data_frame(output_frame);
//                                     kx_session.send_opus_data_frame(output_frame);
//                                     output_frame->free_data_memory();
//                                     output_frame = NULL;
//                                 }
//                             }

//                             /* 发送结束帧 */
//                             kx_session.send_end_frame();
//                             ESP_LOGI(TAG, "说话结束");

//                             /* 等待数据接收完成 */
//                             if (kx_session.wait_recv_done(RECV_TIMEOUT_MS)) {
//                                 ESP_LOGI(TAG, "session 接收数据完成");
//                             } else {
//                                 kx_session.close();
//                                 ESP_LOGE(TAG, "session 接收数据超时");
//                             }
//                             /* 等待播放完成 */
//                             spk_wait_play_done();

//                             /* 恢复录音 */
//                             KxAudio::mic_clear_buffer();
//                             KxAudio::mic_start_record();
//                         }
//                     }
//                 } else {
//                     vTaskDelay(10 / portTICK_PERIOD_MS);
//                 }

//                 event = xEventGroupWaitBits(
//                     event_group,
//                     CHAT_EVENT_EXIT,
//                     pdFALSE,
//                     pdFALSE,
//                     0
//                 );

//                 if (event & CHAT_EVENT_EXIT) {
//                     xEventGroupClearBits(event_group, CHAT_EVENT_WAKEUP);
//                     xEventGroupClearBits(event_group, CHAT_EVENT_EXIT);
//                     break;
//                 }
//             }

//             ESP_LOGI(TAG, "结束对话");
            
//             /* 停止录音 */
//             KxAudio::mic_pause_record();

//             kx_session.close();
//         }

//         vTaskDelay(100 / portTICK_PERIOD_MS);
//     }
// }


void KxChat::chat_task(void *arg) {
    static uint8_t mac_addr[6];
    static char mac_str[18];
    
    KxSession kx_session = KxSession("kx_session");

    kx_audio_frame_t input_frame = {NULL, 0, false};
    kx_audio_frame_t zero_frame = {NULL, 0, false};

    EventBits_t event;

    /* session set ws url */
    esp_efuse_mac_get_default(mac_addr);
    sprintf(mac_str, "%02x:%02x:%02x:%02x:%02x:%02x", 
            mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
    kx_session.set_ws_url((const char*)mac_str);
    ESP_LOGI(TAG, "sessioin ws url: %s", kx_session.get_ws_url());

    /* others init */
    zero_frame.data_size = 512;
    zero_frame.data = (char *)malloc(zero_frame.data_size);
    memset(zero_frame.data, 0x0, zero_frame.data_size);

    for (;;) {
        event = xEventGroupWaitBits(
            event_group,
            CHAT_EVENT_ALL,
            pdFALSE,
            pdFALSE,
            0
        );

        if (event & CHAT_EVENT_RECORD_START) {
            /* 开始录音 */
            if (KxAudio::spk_get_buffer_count() > 0) {
                kx_session.close();
                KxAudio::spk_clear_buffer();
            }

            KxAudio::mic_start_record();
            kx_session.new_session();

            ESP_LOGI(TAG, "开始录音");

            xEventGroupClearBits(event_group, CHAT_EVENT_RECORD_START);
            xEventGroupSetBits(event_group, CHAT_EVENT_RECORDING);

            // 发送开始录音事件
            esp_event_post_to(
                kxchat_event_handle,
                KXCHAT_EVENT,
                KXCHAT_EVENT_RECORD_START,
                NULL,
                0,
                portMAX_DELAY
            );
        }

        if (event & CHAT_EVENT_RECORDING) {
            /* 正在录音 */
            if (KxAudio::mic_pop_voice(&input_frame)) {
                kx_session.send_opus_data_frame(&input_frame);
                input_frame.free_data_memory();
                if (!input_frame.is_end && !(event & CHAT_EVENT_RECORD_END)) {
                    continue;
                }
            }
        }

        if (event & CHAT_EVENT_RECORD_END) {
            /* 录音结束 */
            for (int i = 0; i < 10; i++) {
                if (KxAudio::mic_pop_voice(&input_frame)) {
                    kx_session.send_opus_data_frame(&input_frame);
                    input_frame.free_data_memory();
                }
                vTaskDelay(10 / portTICK_PERIOD_MS);
            }
            for (int i = 0; i < 5; i++) {
                kx_session.send_opus_data_frame(&zero_frame);
            }
            
            KxAudio::mic_pause_record();
            KxAudio::mic_clear_buffer();

            // 发送开始录音结束事件
            esp_event_post_to(
                kxchat_event_handle,
                KXCHAT_EVENT,
                KXCHAT_EVENT_RECORD_END,
                NULL,
                0,
                portMAX_DELAY
            );

            kx_session.send_end_frame();

            ESP_LOGI(TAG, "录音结束");

            xEventGroupClearBits(event_group, CHAT_EVENT_RECORDING);
            xEventGroupClearBits(event_group, CHAT_EVENT_RECORD_END);
        }

        if (KxAudio::spk_get_buffer_count() > 0) {
            xEventGroupSetBits(event_group, CHAT_EVENT_SPEAKING);
        }
        else {
            xEventGroupClearBits(event_group, CHAT_EVENT_SPEAKING);
        }
        
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}


void KxChat::register_event_handler(kxchat_event_t event_id, esp_event_handler_t event_handler, void *event_handler_arg) {
    esp_event_handler_register_with(kxchat_event_handle, KXCHAT_EVENT, event_id, event_handler, event_handler_arg);
}
