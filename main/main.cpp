#include "main.hpp"

/* ESP IDF */
#include "esp_bt.h"
#include "esp_blufi.h"
#include "freertos/FreeRTOS.h"

/* app include */
#include "app_callbacks.hpp"
#include "dto.hpp"
#include "blink.hpp"

/* componetns */
#include "fota.hpp"
#include "netcm.hpp"
#include "kxchat.hpp"
#include "kxchat/kxaudio.hpp"
#include "kxmqtt.hpp"
#include "kxbt.h"
#include "kxfiles.h"

#include "kxsession.hpp"

static const char TAG[] = "main";


BSP_KxDemo bsp_kxdemo;


extern const char test_wav_start[] asm("_binary_test_wav_start");
extern const char test_wav_end[] asm("_binary_test_wav_end");


static void bsp_setup(void) {
    /* utils component init */
    bsp_kxdemo.utils_nvs_flash_init();

    /* driver init */
    bsp_kxdemo.i2c_init();
    bsp_kxdemo.i2s_init();

    /* audio */
    bsp_kxdemo.audio_mic_dev_init();
    bsp_kxdemo.audio_spk_dev_init();

    /* led */
    bsp_kxdemo.led_init();
    bsp_kxdemo.led_set_rgb_all(10, 10, 10);

    /* battery */
    bsp_kxdemo.battery_init();

    /* button */
    bsp_kxdemo.button_play_init();
    bsp_kxdemo.button_play_register_cb(BUTTON_PRESS_DOWN, NULL, button_play_event_cb, NULL);
    bsp_kxdemo.button_play_register_cb(BUTTON_PRESS_UP, NULL, button_play_event_cb, NULL);
    bsp_kxdemo.button_play_register_cb(BUTTON_SINGLE_CLICK, NULL, button_play_event_cb, NULL);
    bsp_kxdemo.button_play_register_cb(BUTTON_DOUBLE_CLICK, NULL, button_play_event_cb, NULL);
    bsp_kxdemo.button_play_register_cb(BUTTON_LONG_PRESS_START, NULL, button_play_event_cb, NULL);
    bsp_kxdemo.button_play_register_cb(BUTTON_LONG_PRESS_UP, NULL, button_play_event_cb, NULL);
}


static void app_load_components() {
    /* led blink */
    LedBlink::init();

    /* kx audio */
    ESP_LOGI(TAG, "in_dev init");
    esp_codec_dev_handle_t in_dev = bsp_kxdemo.audio_get_mic_dev();
    ESP_LOGI(TAG, "out_dev init");
    esp_codec_dev_handle_t out_dev = bsp_kxdemo.audio_get_spk_dev();

    KxAudio::kx_audio_init(in_dev, out_dev);
    KxAudio::kx_start_service(0, 1);

    KxAudio::kx_mic_set_gain(50);
    KxAudio::kx_spk_set_volume(80);

    KxAudio::play_init();

    /* kx chat */
    KxChat::init();
    KxChat::start_chat_task(2, 1);
    KxChat::register_event_handler(KXCHAT_EVENT_ANY, kx_chat_event_handler, NULL);
    // KxChat::pause_chat_task();

    /* 网络管理模块 */
    Netcm::netcm_init();
    Netcm::register_callback(new AppNetcmCallback());
    Netcm::loop_start(5, 0); 

    /* 蓝牙模块 */
    kxbt_init();
    kxbt_blufi_init(Netcm::blufi_get_event_cb());

    /* 固件升级模块 */
    Fota::init();
    Fota::register_event_handler(app_ota_progress_cb, NULL);

    /* mqtt 服务 */
    KxMQTT::init_mqtt(Netcm::sta_get_mac_str());
    KxMQTT::register_callback(new AppMqttCallback());
}


static void idle_task(void *args) {
    for (;;) {
        /* led 灯状态变化 */
        if (Fota::is_in_progress()) {
            LedBlink::green_marquee();
        }
        else if (Netcm::ip_is_got() && Netcm::sta_is_connected()) {
            // if (KxChat::is_wake_up()) {
            //     LedBlink::blue_slow_flashing();
            // }
            // else {
                // LedBlink::green_all();
            // }
            if (KxChat::is_speaking()) {
                LedBlink::blue_fast_flashing();
            }
            else if (KxChat::is_recording()) {
                LedBlink::blue_slow_flashing();
            }
            else {
                LedBlink::green_all();
            }
        }
        else {
            if (Netcm::sta_is_connected()) {
                LedBlink::green_slow_flashing();
            }
            else {
                LedBlink::red_slow_flashing();
            }
        }

        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}


extern "C" void app_main(void)
{
    /* bsp setup */
    bsp_setup();

    /* load components */
    app_load_components();

    /*  idle task create */
    xTaskCreate(&idle_task, "idle_task", 2048, NULL, 5, NULL);
}
