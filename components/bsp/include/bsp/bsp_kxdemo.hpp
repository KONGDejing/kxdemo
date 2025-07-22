
#pragma once


#include "bsp_port.hpp"

/* freertos */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* driver */
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"

/* idf components */
#include "led_strip.h"
#include "led_strip_rmt.h"
#include "button_gpio.h"
#include "iot_button.h"


class BSP_KxDemo : public BSP_Port {
public:
    /* driver */
    void i2c_init(void);
    void i2s_init(void);

    /* audio */
    esp_err_t audio_mic_dev_init(void);
    esp_err_t audio_spk_dev_init(void);

    /* led */
    esp_err_t led_init(void);
    esp_err_t led_set_rgb(uint8_t index, uint8_t r, uint8_t g, uint8_t b);
    esp_err_t led_set_rgb_all(const uint8_t r, const uint8_t g, const uint8_t b);
    esp_err_t led_set_rgb_no_refresh(uint8_t index, uint8_t r, uint8_t g, uint8_t b);
    esp_err_t led_refresh(void);
    
    /* battery */
    esp_err_t battery_init(void);
    float battery_get_voltage(void);
    int battery_get_level(void);

    /* button */
    esp_err_t button_play_init(void);
    esp_err_t button_play_register_cb(button_event_t event, button_event_args_t *event_args, 
        button_cb_t cb, void *usr_data);
    
protected:

private:
    adc_oneshot_unit_handle_t adc1_handle;
    adc_cali_handle_t adc1_cali_chan9_handle;
    bool do_calibration1_chan9 = false;

    led_strip_handle_t led_strip_handle;

    button_handle_t btn_play_handle;
};
