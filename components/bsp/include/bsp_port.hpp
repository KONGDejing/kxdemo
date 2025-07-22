
#pragma once

#include <stdint.h>
#include <stdio.h>

#include "sdkconfig.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_check.h"

#include "driver/i2s_common.h"
#include "driver/i2s_std.h"
#include "esp_codec_dev.h"
#include "esp_codec_dev_defaults.h"

#include "driver/i2c_types.h"
#include "driver/i2c_master.h"

#if CONFIG_BSP_ENABLE_DISPLAY
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lvgl_port_disp.h"
#endif

#define SAMPLE_RATE_HZ 16000


#define BSP_I2C_NUM 0
#define BSP_I2S_NUM 0


#define BSP_SPIFFS_MOUNT_POINT      "/spiffs"
#define BSP_SPIFFS_PARTITION_LABEL  "storage"
#define BSP_SPIFFS_MAX_FILES        5


class BSP_Port {
public:
    /* utils */
    static esp_err_t utils_nvs_flash_init(void);
    static esp_err_t utils_spiffs_mount(void);

    /* driver */
    virtual void i2c_init(void) = 0;
    i2c_master_bus_handle_t i2c_get_handle(void) { return i2c_handle; }
    esp_err_t i2c_device_probe(uint8_t addr);

    virtual void i2s_init(void) = 0;
    i2s_chan_handle_t i2s_get_tx_chan(void) { return i2s_tx_chan; }
    i2s_chan_handle_t i2s_get_rx_chan(void) { return i2s_rx_chan; }

    /* audio */
    virtual esp_err_t audio_mic_dev_init(void) = 0;
    virtual esp_err_t audio_spk_dev_init(void) = 0;
    esp_codec_dev_handle_t audio_get_mic_dev(void) { return audio_mic_dev; }
    esp_codec_dev_handle_t audio_get_spk_dev(void) { return audio_spk_dev; }

#if CONFIG_BSP_ENABLE_DISPLAY
    /* display */
    virtual esp_err_t display_backlight_init(void) = 0;
    virtual esp_err_t display_backlight_set(uint8_t brightness) = 0;
    void display_backlight_on(void) { display_backlight_set(100); }
    void display_backlight_off(void) { display_backlight_set(0); }
    
    virtual esp_err_t display_panel_io_init(void) = 0;
    esp_lcd_panel_io_handle_t display_get_io(void) { return lcd_io; }
    esp_lcd_panel_handle_t display_get_panel(void) { return lcd_panel; }
#endif

protected:
    i2c_master_bus_handle_t i2c_handle = NULL;

#if CONFIG_BSP_ENABLE_DISPLAY
    esp_lcd_panel_io_handle_t lcd_io = NULL;
    esp_lcd_panel_handle_t lcd_panel = NULL;
    lv_display_t *lvgl_disp = NULL;
#endif

    esp_err_t bsp_i2s_init(const i2s_std_config_t *i2s_config);

    esp_codec_dev_handle_t audio_mic_dev = NULL;
    esp_codec_dev_handle_t audio_spk_dev = NULL;

    i2s_chan_handle_t i2s_tx_chan = NULL;
    i2s_chan_handle_t i2s_rx_chan = NULL;
    const audio_codec_data_if_t *i2s_data_if = NULL;

private:

};
