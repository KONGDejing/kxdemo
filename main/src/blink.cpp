#include "blink.hpp"

#include "main.hpp"

#define EVENT_ALL                   (0XFFFFFF)

#define EVENT_RED_SLOW_FLASHING     (BIT0)
#define EVENT_GREEN_SLOW_FLASHING   (BIT1)
#define EVENT_BLUE_SLOW_FLASHING    (BIT2)

#define EVENT_RED_ALL               (BIT3)
#define EVENT_GREEN_ALL             (BIT4)
#define EVENT_BLUE_ALL              (BIT5)

#define EVENT_RED_FAST_FLASHING     (BIT6)
#define EVENT_GREEN_FAST_FLASHING   (BIT7)
#define EVENT_BLUE_FAST_FLASHING    (BIT8)

#define EVENT_RED_MARQUEE           (BIT9)
#define EVENT_GREEN_MARQUEE         (BIT10)
#define EVENT_BLUE_MARQUEE          (BIT11)

#define SET_LED_MODE(X) do { xEventGroupClearBits(gl_event_group, EVENT_ALL);               \
                             xEventGroupSetBits(gl_event_group, X);   \
                            } while(0);

TaskHandle_t gl_flashing_handle;
EventGroupHandle_t gl_event_group;


static void led_red_flashing(int interval_ms) {
    bsp_kxdemo.led_set_rgb_all(10, 0, 0);
    vTaskDelay(interval_ms / 2 / portTICK_PERIOD_MS);
    bsp_kxdemo.led_set_rgb_all(0, 0, 0);
    vTaskDelay(interval_ms / 2 / portTICK_PERIOD_MS);
}


static void led_green_flashing(int interval_ms) {
    bsp_kxdemo.led_set_rgb_all(0, 10, 0);
    vTaskDelay(interval_ms / 2 / portTICK_PERIOD_MS);
    bsp_kxdemo.led_set_rgb_all(0, 0, 0);
    vTaskDelay(interval_ms / 2 / portTICK_PERIOD_MS);
}


static void led_blue_flashing(int interval_ms) {
    bsp_kxdemo.led_set_rgb_all(0, 0, 10);
    vTaskDelay(interval_ms / 2 / portTICK_PERIOD_MS);
    bsp_kxdemo.led_set_rgb_all(0, 0, 0);
    vTaskDelay(interval_ms / 2 / portTICK_PERIOD_MS);
}


static void led_green_marquee(int ms) {
    TickType_t interval = ms / 4 / portTICK_PERIOD_MS;

    for (int i = 0; i < 4; i++) {
        bsp_kxdemo.led_set_rgb_no_refresh(0, 0, (i==0) * 10, 0);
        bsp_kxdemo.led_set_rgb_no_refresh(1, 0, (i==1) * 10, 0);
        bsp_kxdemo.led_set_rgb_no_refresh(2, 0, (i==2) * 10, 0);
        bsp_kxdemo.led_set_rgb_no_refresh(3, 0, (i==3) * 10, 0);
        bsp_kxdemo.led_refresh();
        vTaskDelay(interval);
    }
}


void flashing_task(void *arg) {
    EventBits_t event;

    for (;;) {
        event = xEventGroupWaitBits(
            gl_event_group,
            EVENT_ALL,
            pdFALSE,
            pdFALSE,
            0
        );

        if (event & EVENT_RED_SLOW_FLASHING) {
            led_red_flashing(1000);
        }
        else if (event & EVENT_GREEN_SLOW_FLASHING) {
            led_green_flashing(1000);
        }
        else if (event & EVENT_BLUE_SLOW_FLASHING) {
            led_blue_flashing(1000);
        }

        else if (event & EVENT_RED_ALL) {
            bsp_kxdemo.led_set_rgb_all(10, 0, 0);
        }
        else if (event & EVENT_GREEN_ALL) {
            bsp_kxdemo.led_set_rgb_all(0, 10, 0);
        }
        else if (event & EVENT_BLUE_ALL) {
            bsp_kxdemo.led_set_rgb_all(0, 0, 10);
        }

        else if (event & EVENT_RED_FAST_FLASHING) {
            led_red_flashing(400);
        }
        else if (event & EVENT_GREEN_FAST_FLASHING) {
            led_green_flashing(400);
        }
        else if (event & EVENT_BLUE_FAST_FLASHING) {
            led_blue_flashing(400);
        }

        else if (event & EVENT_GREEN_MARQUEE) {
            led_green_marquee(200);
        }

        else {
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }
    }
}


void LedBlink::init() {
    gl_event_group = xEventGroupCreate();
    xTaskCreate(
        flashing_task,
        "flashing_task",
        4096,
        NULL,
        5,
        &gl_flashing_handle
    );
}


void LedBlink::stop() {
    vTaskSuspend(gl_flashing_handle);
}


void LedBlink::start() {
    vTaskResume(gl_flashing_handle);
}


void LedBlink::red_slow_flashing() {
    xEventGroupClearBits(gl_event_group, EVENT_ALL);
    xEventGroupSetBits(gl_event_group, EVENT_RED_SLOW_FLASHING);
}


void LedBlink::green_slow_flashing() {
    xEventGroupClearBits(gl_event_group, EVENT_ALL);
    xEventGroupSetBits(gl_event_group, EVENT_GREEN_SLOW_FLASHING);
}


void LedBlink::blue_slow_flashing() {
    xEventGroupClearBits(gl_event_group, EVENT_ALL);
    xEventGroupSetBits(gl_event_group, EVENT_BLUE_SLOW_FLASHING);
}


void LedBlink::red_all() {
    SET_LED_MODE(EVENT_RED_ALL);
}


void LedBlink::green_all() {
    SET_LED_MODE(EVENT_GREEN_ALL);
}


void LedBlink::blue_all() {
    SET_LED_MODE(EVENT_BLUE_ALL);
}


void LedBlink::red_fast_flashing() {
    SET_LED_MODE(EVENT_RED_FAST_FLASHING);
}


void LedBlink::green_fast_flashing() {
    SET_LED_MODE(EVENT_GREEN_FAST_FLASHING);
}


void LedBlink::blue_fast_flashing() {
    SET_LED_MODE(EVENT_BLUE_FAST_FLASHING);
}


void LedBlink::green_marquee() {
    SET_LED_MODE(EVENT_GREEN_MARQUEE);
}
