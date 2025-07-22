
#pragma once

#include "bsp_port.hpp"


class BSP_Dnesp32S3 : public BSP_Port {
public:

    /* audio */
    void i2s_init();
    esp_codec_dev_handle_t audio_mic_dev_init();
    esp_codec_dev_handle_t audio_spk_dev_init();

protected:

private:

};
