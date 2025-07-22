#include "bsp/bsp_box3.hpp"

// #include "bsp/esp-box-3.h"
#include "driver/gpio.h"
#include "driver/ledc.h"

#include "esp_codec_dev.h"
#include "esp_codec_dev_defaults.h"

#include "esp_lvgl_port.h"


const char TAG[] = "bsp_box3";


/**************************************************************************************************
 *  ESP-BOX pinout
 **************************************************************************************************/
/* I2C */
#define BSP_I2C_SCL           (GPIO_NUM_18)
#define BSP_I2C_SDA           (GPIO_NUM_8)

/* Audio */
#define BSP_I2S_SCLK          (GPIO_NUM_17)
#define BSP_I2S_MCLK          (GPIO_NUM_2)
#define BSP_I2S_LCLK          (GPIO_NUM_45)
#define BSP_I2S_DOUT          (GPIO_NUM_15) // To Codec ES8311
#define BSP_I2S_DSIN          (GPIO_NUM_16) // From ADC ES7210
#define BSP_POWER_AMP_IO      (GPIO_NUM_46)
#define BSP_MUTE_STATUS       (GPIO_NUM_1)

/* Display */
#define BSP_LCD_DATA0         (GPIO_NUM_6)
#define BSP_LCD_PCLK          (GPIO_NUM_7)
#define BSP_LCD_CS            (GPIO_NUM_5)
#define BSP_LCD_DC            (GPIO_NUM_4)
#define BSP_LCD_RST           (GPIO_NUM_48)

#define BSP_LCD_BACKLIGHT     (GPIO_NUM_47)
#define BSP_LCD_TOUCH_INT     (GPIO_NUM_3)

/* USB */
#define BSP_USB_POS           (GPIO_NUM_20)
#define BSP_USB_NEG           (GPIO_NUM_19)

/* Buttons */
#define BSP_BUTTON_CONFIG_IO  (GPIO_NUM_0)
#define BSP_BUTTON_MUTE_IO    (GPIO_NUM_1)

/* uSD card MMC */
#define BSP_SD_D0             (GPIO_NUM_9)
#define BSP_SD_D1             (GPIO_NUM_13)
#define BSP_SD_D2             (GPIO_NUM_42)
#define BSP_SD_D3             (GPIO_NUM_12)
#define BSP_SD_CMD            (GPIO_NUM_14)
#define BSP_SD_CLK            (GPIO_NUM_11)
#define BSP_SD_DET            (GPIO_NUM_NC)
#define BSP_SD_POWER          (GPIO_NUM_43)

/* uSD card SPI */
#define BSP_SD_SPI_MISO       (GPIO_NUM_9)
#define BSP_SD_SPI_CS         (GPIO_NUM_12)
#define BSP_SD_SPI_MOSI       (GPIO_NUM_14)
#define BSP_SD_SPI_CLK        (GPIO_NUM_11)

/* PMOD */
/*
 * PMOD interface (peripheral module interface) is an open standard defined by Digilent Inc.
 * for peripherals used with FPGA or microcontroller development boards.
 *
 * ESP-BOX contains two double PMOD connectors, protected with ESD protection diodes.
 * Power pins are on 3.3V.
 *
 * Double PMOD Connectors on ESP-BOX-3 dock are labeled as follows:
 *      |------------|
 *      | IO1    IO5 |
 *      | IO2    IO6 |
 *      | IO3    IO7 |
 *      | IO4    IO8 |
 *      |------------|
 *      | GND    GND |
 *      | 3V3    3V3 |
 *      |------------|
 */
#define BSP_PMOD1_IO1        GPIO_NUM_42
#define BSP_PMOD1_IO2        BSP_USB_POS
#define BSP_PMOD1_IO3        GPIO_NUM_39
#define BSP_PMOD1_IO4        GPIO_NUM_40 // Intended for I2C SCL (pull-up NOT populated)
#define BSP_PMOD1_IO5        GPIO_NUM_21
#define BSP_PMOD1_IO6        BSP_USB_NEG
#define BSP_PMOD1_IO7        GPIO_NUM_38
#define BSP_PMOD1_IO8        GPIO_NUM_41 // Intended for I2C SDA (pull-up NOT populated)

#define BSP_PMOD2_IO1        GPIO_NUM_13 // Intended for SPI2 Q (MISO)
#define BSP_PMOD2_IO2        GPIO_NUM_9  // Intended for SPI2 HD (Hold)
#define BSP_PMOD2_IO3        GPIO_NUM_12 // Intended for SPI2 CLK
#define BSP_PMOD2_IO4        GPIO_NUM_44 // UART0 RX by default
#define BSP_PMOD2_IO5        GPIO_NUM_10 // Intended for SPI2 CS
#define BSP_PMOD2_IO6        GPIO_NUM_14 // Intended for SPI2 WP (Write-protect)
#define BSP_PMOD2_IO7        GPIO_NUM_11 // Intended for SPI2 D (MOSI)
#define BSP_PMOD2_IO8        GPIO_NUM_43 // UART0 TX by defaultf

void BSP_Box3::i2c_init(void) {
    const i2c_master_bus_config_t i2c_config = {
        .i2c_port = BSP_I2C_NUM,
        .sda_io_num = BSP_I2C_SDA,
        .scl_io_num = BSP_I2C_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_config, &i2c_handle));
}

void BSP_Box3::i2s_init() {
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


esp_err_t BSP_Box3::audio_mic_dev_init(void)
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
        .mic_selected = ES7120_SEL_MIC1 | ES7120_SEL_MIC3,
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


esp_err_t BSP_Box3::audio_spk_dev_init(void)
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

    esp_codec_dev_cfg_t codec_dev_cfg = {
        .dev_type = ESP_CODEC_DEV_TYPE_OUT,
        .codec_if = es8311_dev,
        .data_if = i2s_data_if,
    };
    audio_spk_dev = esp_codec_dev_new(&codec_dev_cfg);
    assert(audio_spk_dev);

    return ESP_OK;
}


esp_err_t BSP_Box3::display_backlight_init(void) {
    // Setup LEDC peripheral for PWM backlight control
    const ledc_channel_config_t LCD_backlight_channel = {
        .gpio_num = BSP_LCD_BACKLIGHT,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = (ledc_channel_t )LCD_LEDC_CH,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = (ledc_timer_t )1,
        .duty = 0,
        .hpoint = 0
    };
    const ledc_timer_config_t LCD_backlight_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .timer_num = (ledc_timer_t )1,
        .freq_hz = 5000,
        .clk_cfg = LEDC_AUTO_CLK
    };

    ESP_ERROR_CHECK(ledc_timer_config(&LCD_backlight_timer));
    ESP_ERROR_CHECK(ledc_channel_config(&LCD_backlight_channel));

    return ESP_OK;
}

esp_err_t BSP_Box3::display_backlight_set(uint8_t brightness) {
    if (brightness > 100) {
        brightness = 100;
    }

    ESP_LOGI(TAG, "Setting LCD backlight: %d%%", brightness);
    uint32_t duty_cycle = (1023 * brightness) / 100; // LEDC resolution set to 10bits, thus: 100% = 1023
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t )LCD_LEDC_CH, duty_cycle));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t )LCD_LEDC_CH));

    return ESP_OK;
}


esp_err_t BSP_Box3::display_panel_io_init(void) {
    ESP_LOGD(TAG, "Initialize SPI bus");
    const spi_bus_config_t buscfg = {
        .mosi_io_num = BSP_LCD_DATA0,
        .miso_io_num = GPIO_NUM_NC,
        .sclk_io_num = BSP_LCD_PCLK,
        .quadwp_io_num = GPIO_NUM_NC,
        .quadhd_io_num = GPIO_NUM_NC,
        .max_transfer_sz = BSP_LCD_H_RES * BSP_LCD_V_RES * 2,
    };
    ESP_RETURN_ON_ERROR(spi_bus_initialize(BSP_LCD_SPI_NUM, &buscfg, SPI_DMA_CH_AUTO), TAG, "SPI init failed");

    ESP_LOGD(TAG, "Install panel IO");
    const esp_lcd_panel_io_spi_config_t io_config = {
        .cs_gpio_num = BSP_LCD_CS,
        .dc_gpio_num = BSP_LCD_DC,
        .spi_mode = 0,
        .pclk_hz = BSP_LCD_PIXEL_CLOCK_HZ,
        .trans_queue_depth = 10,
        .lcd_cmd_bits = LCD_CMD_BITS,
        .lcd_param_bits = LCD_PARAM_BITS,
    };
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)BSP_LCD_SPI_NUM, &io_config, &lcd_io), TAG, "New panel IO failed");

    ESP_LOGD(TAG, "Install LCD driver");
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = BSP_LCD_RST, // Shared with Touch reset
        .color_space = BSP_LCD_COLOR_SPACE,
        .bits_per_pixel = BSP_LCD_BITS_PER_PIXEL,
        .flags = {
            .reset_active_high = 1
        }
    };
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_st7789(lcd_io, (const esp_lcd_panel_dev_config_t *)&panel_config, &lcd_panel), TAG, "New panel failed");

    esp_lcd_panel_reset(lcd_panel);
    esp_lcd_panel_init(lcd_panel);
    esp_lcd_panel_mirror(lcd_panel, true, true);

    esp_lcd_panel_disp_on_off(lcd_panel, true);

    /* Initialize LVGL */
    const lvgl_port_cfg_t lvgl_cfg = {
        .task_priority = 4,
        .task_stack = 7168,
        .task_affinity = 0,
        .task_max_sleep_ms = 500,
        .timer_period_ms = 5
    };
    ESP_RETURN_ON_ERROR(lvgl_port_init(&lvgl_cfg), TAG, "LVGL port initialization failed");

    /* Add LCD screen */
    ESP_LOGD(TAG, "Add LCD screen");
    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = lcd_io,
        .panel_handle = lcd_panel,
        .buffer_size = BSP_LCD_H_RES * 40,
        .double_buffer = false,
        .hres = BSP_LCD_H_RES,
        .vres = BSP_LCD_V_RES,
        .monochrome = false,
        /* Rotation values must be same as used in esp_lcd for initial settings of the screen */
        .rotation = {
            .swap_xy = false,
            .mirror_x = true,
            .mirror_y = true,
        },
        .flags = {
            .buff_dma = true,
            .buff_spiram = false,
#if LVGL_VERSION_MAJOR >= 9
            .swap_bytes = (BSP_LCD_BIGENDIAN ? true : false),
#endif
        }
    };

    lvgl_disp = lvgl_port_add_disp(&disp_cfg);

    return ESP_OK;
}


esp_err_t BSP_Box3::goldfinger_i2c_init(void) {
    i2c_master_bus_config_t i2c_mst_config = {
        #if  BSP_I2C_NUM
            .i2c_port = 0,
        #else
            .i2c_port = 1,
        #endif
        .sda_io_num = GPIO_NUM_41, // BSP_PMOD1_IO8,    // 41
        .scl_io_num = GPIO_NUM_40, // BSP_PMOD1_IO4,    // 40
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags = {
            .enable_internal_pullup = true,
        }
    };

    i2c_master_bus_handle_t bus_handle;
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &bus_handle));

    return ESP_OK;
}