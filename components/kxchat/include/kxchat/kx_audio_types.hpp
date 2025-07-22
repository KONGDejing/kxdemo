
#pragma once

#ifndef __KX_AUDIO_TYPES_HPP__
#define __KX_AUDIO_TYPES_HPP__


#include <stdint.h>
#include <stdio.h>
#include <string.h>


#define MIC_FRAME_CNT           100
#define MIC_FRAME_SIZE          160         // int16_t
#define FRAME_BITS              16
#define SAMPLE_RATE_HZ          16000
#define FRAME_LENGTH_MS         10


typedef struct {
    uint32_t chunk_id;                  // RIFF 52 49 46 46
    uint32_t chunk_size;
    uint32_t format;                    // WAVE 57 41 56 45
} kx_wave_riff_t;


typedef struct 
{
    uint32_t sub_chunk1_id;             // fmt  66 6d 74 20
    uint32_t sub_chunk1_size;
    uint16_t audio_format;              // pcm 0x01
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
} kx_wave_fmt_t;


typedef struct
{
    uint32_t list_chunk_id;
    uint32_t list_chunk_size;
    char list_data[26];
} kx_wave_list_t;


typedef struct {
    uint32_t sub_chunk2_id;             // data 64 61 74 61
    uint32_t sub_chunk2_size;
}kx_wave_data_t;


typedef enum {
    KX_AUDIO_FRAME_CMD_NONE = 0,
    KX_AUDIO_FRAME_CMD_STOP,
    KX_AUDIO_FRAME_CMD_PAUSE,
    KX_AUDIO_FRAME_CMD_RESUME,
    KX_AUDIO_FRAME_CMD_MP3_FILE,
    KX_AUDIO_FRAME_CMD_MP3_URL,
}kx_audio_frame_cmd_t;


typedef struct
{
    char *data;
    int data_size;
    bool is_end;

    kx_audio_frame_cmd_t cmd;

    void free_data_memory() {
        if (NULL != data) {
            data_size = 0;
            free(data);
            data = NULL;
        }
    }
}kx_audio_frame_t;


typedef struct kx_audio_frame_ring_queue {
    kx_audio_frame_t *frames;
    int head;
    int tail;
    int size;
    int max_size;

    kx_audio_frame_ring_queue(int max_size) {
        frames = (kx_audio_frame_t *)calloc(max_size, sizeof(kx_audio_frame_t));
        head = 0;
        tail = 0;
        size = 0;
        this->max_size = max_size;
    }

    bool push(kx_audio_frame_t *frame) {
        if (size >= max_size) return false;
        frames[tail] = *frame;
        tail = (tail + 1) % max_size;
        size++;
        return true;
    }

    bool overwrite(kx_audio_frame_t *frame) {
        if (size >= max_size) {
            frames[head].free_data_memory();
            head = (head + 1) % max_size;
            size--;
        }
        frames[tail] = *frame;
        tail = (tail + 1) % max_size;
        size++;
        return true;
    }

    kx_audio_frame_t *pop() {
        if (size <= 0) return NULL;
        kx_audio_frame_t *frame = &frames[head];
        head = (head + 1) % max_size;
        size--;
        return frame;
    }

    bool is_full() {
        return (size >= max_size);
    }

    bool is_empty() {
        return (size <= 0);
    }

    void clear() {
        head = 0;
        tail = 0;
        size = 0;
    }

} kx_audio_frame_ring_queue_t;


int get_wave_head_offset(const char *data_ptr, size_t len);
kx_audio_frame_t create_audio_frame_from_bytes(const char *data_ptr, size_t len);


#endif /* __KX_AUDIO_TYPES_HPP__ */
