
#pragma once


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "esp_event.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/idf_additions.h"

#include "esp_codec_dev.h"


typedef enum {
    KXCHAT_EVENT_ANY = -1,
    KXCHAT_EVENT_RECORD_START,
    KXCHAT_EVENT_RECORD_END,
    KXCHAT_EVENT_CHANGE_VOLUME
}kxchat_event_t;

ESP_EVENT_DECLARE_BASE(KXCHAT_EVENT);

class KxChat {
public:
    static void init();
    static void start_chat_task(int priority, int core_id);

    // static void pause_chat_task();
    // static void resume_chat_task();
    // static bool is_chat_paused();

    // static void start_chat(void);
    // static void exit_chat(void);
    // static bool is_wake_up(void);

    static void start_record(void);
    static void stop_record(void);
    static bool is_recording();
    static bool is_speaking(void);

    static void register_event_handler(kxchat_event_t event_id, esp_event_handler_t event_handler, void *event_handler_arg);
    static esp_event_loop_handle_t kxchat_event_handle;
private:
    static StackType_t *chat_task_stack;
    static StaticTask_t chat_task_tcb;
    static TaskHandle_t chat_task_handle;
    static EventGroupHandle_t event_group;

    static void spk_wait_play_done();
    static void chat_task(void *arg);

protected:

};