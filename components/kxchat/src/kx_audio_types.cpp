#include "kxchat/kx_audio_types.hpp"

#include "esp_heap_caps.h"
#include "esp_log.h"


static const char TAG[] = "audio_types";


int get_wave_head_offset(const char *data_ptr, size_t len) {
    kx_wave_riff_t *riff = (kx_wave_riff_t*)data_ptr;

    if (len < 44) {
        return 0;
    }

    if (riff->chunk_id != 0x46464952) {
        return 0;
    }

    if (riff->format != 0x45564157) {
        return 0;
    }

    if (riff->chunk_size == 0xFFFFFFFF) {
        return 78;
    }

    return 44;
}


kx_audio_frame_t create_audio_frame_from_bytes(const char *data_ptr, size_t len) {
    kx_audio_frame_t frame = {NULL, 0};
    int data_offset = 0;

    // 数据偏移设置
    data_offset = get_wave_head_offset(data_ptr, len);
    if (data_offset > 0) {
        ESP_LOGW(TAG, "wave head offset: %d", data_offset);
    }

    // 内存申请
    frame.data_size = len - data_offset;
    frame.data = (char*)heap_caps_malloc(frame.data_size, MALLOC_CAP_SPIRAM);
    // 复制数据
    memcpy(frame.data, data_ptr + data_offset, frame.data_size);

    return frame;
}
