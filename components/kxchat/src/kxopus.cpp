#include "kxchat/kxopus.hpp"

#include "esp_log.h"
#include "esp_err.h"


static const char *TAG = "opus";


esp_audio_dec_handle_t KxOpusCodec::decoder = NULL;
esp_audio_enc_handle_t KxOpusCodec::encoder = NULL;


void KxOpusCodec::init() {
    esp_opus_dec_cfg_t opus_dec_cfg = {
        .sample_rate = ESP_AUDIO_SAMPLE_RATE_16K,
        .channel = ESP_AUDIO_MONO,
        .frame_duration = ESP_OPUS_DEC_FRAME_DURATION_60_MS,
        .self_delimited = false
    };
    ESP_ERROR_CHECK(esp_opus_dec_open(&opus_dec_cfg, sizeof(opus_dec_cfg), &decoder));
    if (decoder == NULL) {
        ESP_LOGE(TAG, "Failed to open decoder");
    }

    esp_opus_enc_config_t opus_enc_cfg = {
        .sample_rate = ESP_AUDIO_SAMPLE_RATE_16K,
        .channel = ESP_AUDIO_MONO,
        .bits_per_sample = ESP_AUDIO_BIT16,
        .bitrate = 40000,
        .frame_duration = ESP_OPUS_ENC_FRAME_DURATION_60_MS,
        .application_mode = ESP_OPUS_ENC_APPLICATION_AUDIO,
        .complexity = 0,
        .enable_fec = false,
        .enable_dtx = false,
        .enable_vbr = false
    };
    ESP_ERROR_CHECK(esp_opus_enc_open(&opus_enc_cfg, sizeof(opus_enc_cfg), &encoder)); 
    if (encoder == NULL) {
        ESP_LOGE(TAG, "Failed to open encoder");
    }
}


void KxOpusCodec::decode(uint8_t *data, uint32_t len, uint8_t *out, uint32_t out_len) {
    esp_audio_dec_in_raw_t raw = {
        .buffer = data,
        .len = len
    };

    esp_audio_dec_out_frame_t out_frame = {
        .buffer = out,
        .len = out_len
    };

    esp_audio_dec_info_t info;

    esp_err_t ret = ESP_AUDIO_ERR_OK;

    while (raw.len)
    {
        ret = esp_opus_dec_decode(decoder, &raw, &out_frame, &info);
        if (ret != ESP_AUDIO_ERR_OK) {
            break;
        }
        raw.buffer += raw.consumed;
        raw.len -= raw.consumed;
    }
}


void KxOpusCodec::encode(uint8_t *data, uint32_t len, esp_audio_enc_out_frame_t *out) {
    esp_audio_enc_in_frame_t in_frame = {
        .buffer = data,
        .len = len
    };
    esp_opus_enc_process(encoder, &in_frame, out);
}


void KxOpusCodec::get_frame_size(int *in_size, int *out_size) {
    esp_opus_enc_get_frame_size(encoder, in_size, out_size);
}
