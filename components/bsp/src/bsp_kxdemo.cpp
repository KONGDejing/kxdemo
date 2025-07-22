#include "bsp/bsp_kxdemo.hpp"

#include "driver/gpio.h"

#include "hal/adc_types.h"
#include "esp_adc/adc_cali_scheme.h"

#include "esp_codec_dev.h"
#include "esp_codec_dev_defaults.h"


const char TAG[] = "bsp_kxdemo";


#define BSP_I2C_SDA         (GPIO_NUM_8)
#define BSP_I2C_SCL         (GPIO_NUM_18)

#define BSP_I2S_MCLK        (GPIO_NUM_2)
#define BSP_I2S_SCLK        (GPIO_NUM_17)
#define BSP_I2S_LCLK        (GPIO_NUM_45)
#define BSP_I2S_DOUT        (GPIO_NUM_15) // To Codec ES8311
#define BSP_I2S_DSIN        (GPIO_NUM_16) // From ADC ES7210

#define BSP_PA_CTRL         (GPIO_NUM_46)
#define BSP_POWER_AMP_IO    (BSP_PA_CTRL)

#define BSP_PLAY_KEY        (GPIO_NUM_1)
#define BSP_SD_DAT2         (GPIO_NUM_42)
#define BSP_EXT_KEY_3       (GPIO_NUM_41)
#define BSP_EXT_KEY_2       (GPIO_NUM_40)
#define BSP_EXT_KEY_1       (GPIO_NUM_39)

#define BSP_MCU_BOOT        (GPIO_NUM_0)
#define BSP_BAT_ADC         (GPIO_NUM_10) // SR ADC ADC1_CHANNEL_9

#define BSP_LED_IO          (GPIO_NUM_38) // XL-3528RGBW-WS2812B
#define BSP_LED_NUM         (4)


void BSP_KxDemo::i2c_init(void) {
    const i2c_master_bus_config_t i2c_config = {
        .i2c_port = BSP_I2C_NUM,
        .sda_io_num = BSP_I2C_SDA,
        .scl_io_num = BSP_I2C_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .intr_priority = 0,
        .trans_queue_depth = 0, 
        .flags = 0
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_config, &i2c_handle));
}


void BSP_KxDemo::i2s_init(void) {
    i2s_std_config_t i2s_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE_HZ),
        .slot_cfg = I2S_STD_PHILIP_SLOT_DEFAULT_CONFIG(
            I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = BSP_I2S_MCLK,
            .bclk = BSP_I2S_SCLK,
            .ws = BSP_I2S_LCLK,    
            .dout = BSP_I2S_DOUT,  
            .din = BSP_I2S_DSIN,   
            .invert_flags = {      
                .mclk_inv = false, 
                .bclk_inv = false, 
                .ws_inv = false,   
            }
        }
    };
    bsp_i2s_init(&i2s_cfg);
    ESP_LOGI(TAG, "i2s init");
}


esp_err_t BSP_KxDemo::audio_mic_dev_init(void)
{
    // const audio_codec_data_if_t *i2s_data_if = bsp_audio_get_codec_itf();
    assert(i2s_data_if);

    i2c_master_bus_handle_t i2c_handle = NULL;
    i2c_master_get_bus_handle(BSP_I2C_NUM, &i2c_handle);

    audio_codec_i2c_cfg_t i2c_cfg = {
        .port = BSP_I2C_NUM,
        .addr = ES7210_CODEC_DEFAULT_ADDR,
        .bus_handle = i2c_handle,
    };
    const audio_codec_ctrl_if_t *i2c_ctrl_if = audio_codec_new_i2c_ctrl(&i2c_cfg);
    assert(i2c_ctrl_if);

    es7210_codec_cfg_t es7210_cfg = {
        .ctrl_if = i2c_ctrl_if,
        // .mic_selected = ES7120_SEL_MIC1 | ES7120_SEL_MIC2 | ES7120_SEL_MIC3 | ES7120_SEL_MIC4,
        .mic_selected = ES7120_SEL_MIC1 | ES7120_SEL_MIC2,
    };
    const audio_codec_if_t *es7210_dev = es7210_codec_new(&es7210_cfg);
    assert(es7210_dev);

    ESP_LOGI(TAG, "es7210 codec init");

    esp_codec_dev_cfg_t codec_es7210_dev_cfg = {
        .dev_type = ESP_CODEC_DEV_TYPE_IN,
        .codec_if = es7210_dev,
        .data_if = i2s_data_if,
    };
    audio_mic_dev = esp_codec_dev_new(&codec_es7210_dev_cfg);
    assert(audio_mic_dev);

    return ESP_OK;
}


esp_err_t BSP_KxDemo::audio_spk_dev_init(void)
{
    // const audio_codec_data_if_t *i2s_data_if = bsp_audio_get_codec_itf();
    assert(i2s_data_if);

    const audio_codec_gpio_if_t *gpio_if = audio_codec_new_gpio();
    i2c_master_bus_handle_t i2c_handle = NULL;
    i2c_master_get_bus_handle(BSP_I2C_NUM, &i2c_handle);

    audio_codec_i2c_cfg_t i2c_cfg = {
        .port = BSP_I2C_NUM,
        .addr = ES8311_CODEC_DEFAULT_ADDR,
        .bus_handle = i2c_handle,
    };
    const audio_codec_ctrl_if_t *i2c_ctrl_if = audio_codec_new_i2c_ctrl(&i2c_cfg);
    assert(i2c_ctrl_if);

    esp_codec_dev_hw_gain_t gain = {
        .pa_voltage = 5.0,
        .codec_dac_voltage = 3.3,
    };

    es8311_codec_cfg_t es8311_cfg = {
        .ctrl_if = i2c_ctrl_if,
        .gpio_if = gpio_if,
        .codec_mode = ESP_CODEC_DEV_WORK_MODE_DAC,
        .pa_pin = BSP_POWER_AMP_IO,
        .pa_reverted = false,
        .master_mode = false,
        .use_mclk = true,
        .digital_mic = false,
        .invert_mclk = false,
        .invert_sclk = false,
        .hw_gain = gain,
    };
    const audio_codec_if_t *es8311_dev = es8311_codec_new(&es8311_cfg);
    assert(es8311_dev);

    ESP_LOGI(TAG, "es8311 codec init");

    esp_codec_dev_cfg_t codec_dev_cfg = {
        .dev_type = ESP_CODEC_DEV_TYPE_OUT,
        .codec_if = es8311_dev,
        .data_if = i2s_data_if,
    };
    audio_spk_dev = esp_codec_dev_new(&codec_dev_cfg);
    assert(audio_spk_dev);

    return ESP_OK;
}

/* led */

esp_err_t BSP_KxDemo::led_init(void) {
    /// LED strip common configuration
    led_strip_config_t strip_config = {
        .strip_gpio_num = BSP_LED_IO,  // The GPIO that connected to the LED strip's data line
        .max_leds = BSP_LED_NUM,       // The number of LEDs in the strip,
        .led_model = LED_MODEL_WS2812, // LED strip model, it determines the bit timing
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB, // The color component format is G-R-B
        .flags = {
            .invert_out = false, // don't invert the output signal
        }
    };

    /// RMT backend specific configuration
    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,    // different clock source can lead to different power consumption
        .resolution_hz = 10 * 1000 * 1000, // RMT counter clock frequency: 10MHz
        .mem_block_symbols = 64,           // the memory size of each RMT channel, in words (4 bytes)
        .flags = {
            .with_dma = true, // DMA feature is available on chips like ESP32-S3/P4
        }
    };

    /// Create the LED strip object
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip_handle));

    return ESP_OK;
}


esp_err_t BSP_KxDemo::led_set_rgb(uint8_t index, uint8_t r, uint8_t g, uint8_t b) {
    led_strip_set_pixel(led_strip_handle, index, r, g, b);
    return led_strip_refresh(led_strip_handle);
}


esp_err_t BSP_KxDemo::led_set_rgb_all(const uint8_t r, const uint8_t g, const uint8_t b) {
    for (int i = 0; i < BSP_LED_NUM; i++) { 
        led_strip_set_pixel(led_strip_handle, i, r, g, b);
    }
    return led_strip_refresh(led_strip_handle);
}


esp_err_t BSP_KxDemo::led_set_rgb_no_refresh(uint8_t index, uint8_t r, uint8_t g, uint8_t b) {
    led_strip_set_pixel(led_strip_handle, index, r, g, b);
    return ESP_OK;
}


esp_err_t BSP_KxDemo::led_refresh(void) {
    return led_strip_refresh(led_strip_handle);
}


/* battery */


static bool adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle)
{
    adc_cali_handle_t handle = NULL;
    esp_err_t ret = ESP_FAIL;
    bool calibrated = false;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    if (!calibrated) {
        ESP_LOGI(TAG, "calibration scheme version is %s", "Curve Fitting");
        adc_cali_curve_fitting_config_t cali_config = {
            .unit_id = unit,
            .chan = channel,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
#endif

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    if (!calibrated) {
        ESP_LOGI(TAG, "calibration scheme version is %s", "Line Fitting");
        adc_cali_line_fitting_config_t cali_config = {
            .unit_id = unit,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
#endif

    *out_handle = handle;
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Calibration Success");
    } else if (ret == ESP_ERR_NOT_SUPPORTED || !calibrated) {
        ESP_LOGW(TAG, "eFuse not burnt, skip software calibration");
    } else {
        ESP_LOGE(TAG, "Invalid arg or no memory");
    }

    return calibrated;
}


esp_err_t BSP_KxDemo::battery_init(void) {
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    adc_oneshot_chan_cfg_t config = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_9, &config));

    do_calibration1_chan9 = adc_calibration_init(ADC_UNIT_1, ADC_CHANNEL_9, ADC_ATTEN_DB_12, &adc1_cali_chan9_handle);
    return ESP_OK;
}


float BSP_KxDemo::battery_get_voltage(void) {
    int adc_reading = 0;
    int voltage = 0;
    float voltage_f = 0;
    float battery_voltage = 0;
    
    ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, ADC_CHANNEL_9, &adc_reading));
    
    if (do_calibration1_chan9) {
        ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc1_cali_chan9_handle, adc_reading, &voltage));
        voltage_f = voltage / 1000.0f;
    } else {
        voltage_f = adc_reading * 3.3f / 4095.0f;
    }

    battery_voltage = voltage_f * 4.0f;

    ESP_LOGD(TAG, "ADC reading: %d ADC voltage: %.2f ", adc_reading, voltage_f);
    ESP_LOGD(TAG, "Battery voltage: %.2fV", battery_voltage);

    return battery_voltage;
}


int BSP_KxDemo::battery_get_level(void) {
        float voltage = battery_get_voltage();
    if (voltage <= 3.0f) {
        return 0;
    }
    else if (voltage <= 3.45f) {
        return 5;
    }
    else if (voltage <= 3.50f) {
        return 10;
    }
    else if (voltage <= 3.55f) {
        return 15;
    }
    else if (voltage <= 3.60f) {
        return 20;
    }
    else if (voltage <= 3.65f) {
        return 25;
    }
    else if (voltage <= 3.77f) {
        return 30;
    }
    else if (voltage <= 3.79f) {
        return 40;
    }
    else if (voltage <= 3.87f) {
        return 60;
    }
    else if (voltage <= 3.92f) {
        return 70;
    }
    else if (voltage <= 3.98f) {
        return 80;
    }
    else if (voltage <= 4.00f) {
        return 90;
    }
    else {
        return 100;
    }
}


esp_err_t BSP_KxDemo::button_play_init(void) {
    button_config_t btn_cfg = {0};
    button_gpio_config_t gpio_cfg = {
        .gpio_num = BSP_PLAY_KEY,
        .active_level = 0,
        .enable_power_save = false,
        .disable_pull = false
    };

    
    return iot_button_new_gpio_device(&btn_cfg, &gpio_cfg, &btn_play_handle);
}


esp_err_t BSP_KxDemo::button_play_register_cb(button_event_t event, button_event_args_t *event_args, button_cb_t cb, void *usr_data) {
    return iot_button_register_cb(btn_play_handle, event, event_args, cb, usr_data);
}
