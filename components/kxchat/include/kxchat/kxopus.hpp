
#pragma once

#include <stdint.h>

#include "decoder/impl/esp_opus_dec.h"
#include "encoder/impl/esp_opus_enc.h"


class KxOpusCodec {
public:
    static void init();
    static void decode(uint8_t *data, uint32_t len, uint8_t *out, uint32_t out_len);
    static void encode(uint8_t *data, uint32_t len, esp_audio_enc_out_frame_t *out);
    static void get_frame_size(int *in_size, int *out_size);

private:
    static esp_audio_dec_handle_t decoder;
    static esp_audio_enc_handle_t encoder;

};