
#pragma once

#include "bsp_port.hpp"


/* LCD color formats */
#define ESP_LCD_COLOR_FORMAT_RGB565    (1)
#define ESP_LCD_COLOR_FORMAT_RGB888    (2)

/* LCD display color format */
#define BSP_LCD_COLOR_FORMAT        (ESP_LCD_COLOR_FORMAT_RGB565)
/* LCD display color bytes endianess */
#define BSP_LCD_BIGENDIAN           (1)
/* LCD display color bits */
#define BSP_LCD_BITS_PER_PIXEL      (16)
/* LCD display color space */
#define BSP_LCD_COLOR_SPACE         (ESP_LCD_COLOR_SPACE_BGR)
/* LCD definition */
#define BSP_LCD_H_RES              (320)
#define BSP_LCD_V_RES              (240)

#define BSP_LCD_PIXEL_CLOCK_HZ     (40 * 1000 * 1000)
#define BSP_LCD_SPI_NUM             SPI3_HOST

#define LCD_CMD_BITS           8
#define LCD_PARAM_BITS         8
#define LCD_LEDC_CH            1

class BSP_Box3 : public BSP_Port {
public:
    /* driver */
    void i2c_init(void);
    void i2s_init(void);

    /* audio */
    esp_err_t audio_mic_dev_init(void);
    esp_err_t audio_spk_dev_init(void);

    /* display */
    esp_err_t display_backlight_init(void);
    esp_err_t display_backlight_set(uint8_t brightness);

    esp_err_t display_panel_io_init(void);

    /* other */
    esp_err_t goldfinger_i2c_init(void);

protected:

private:

};
