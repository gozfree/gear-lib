/******************************************************************************
 * Copyright (C) 2014-2020 Zhifeng Gong <gozfree@163.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 ******************************************************************************/
#ifndef AUDIO_DEF_H
#define AUDIO_DEF_H

#include <stdint.h>

/**
 * This file reference to ffmpeg and obs define
 */

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * uncompressed audio define
 ******************************************************************************/

enum sample_format {
    SAMPLE_FORMAT_UNKNOWN,

    SAMPLE_FORMAT_U8BIT,
    SAMPLE_FORMAT_16BIT,
    SAMPLE_FORMAT_32BIT,
    SAMPLE_FORMAT_FLOAT,

    SAMPLE_FORMAT_U8BIT_PLANAR,
    SAMPLE_FORMAT_16BIT_PLANAR,
    SAMPLE_FORMAT_32BIT_PLANAR,
    SAMPLE_FORMAT_FLOAT_PLANAR,

    SAMPLE_FORMAT_CNT,
};

#ifndef AUDIO_MAX_CHANNELS
#define AUDIO_MAX_CHANNELS 8
#endif

struct audio_frame {
    uint8_t           *data[AUDIO_MAX_CHANNELS];
    uint32_t           frames;
    enum sample_format format;
    uint32_t           sample_rate;
    uint64_t           timestamp;//ns
    uint64_t           frame_id;
    uint64_t           total_size;
};

/******************************************************************************
 * compressed audio define
 ******************************************************************************/
enum audio_codec_format {
    AUDIO_CODEC_AAC,
    AUDIO_CODEC_G711_A,
    AUDIO_CODEC_G711_U,

    AUDIO_CODEC_CNT,
};

/**
 * This structure describe encoder attribute
 */
struct audio_encoder {
    enum audio_codec_format format;
    uint32_t                sample_rate;
    uint32_t                sample_size;
    int                     channels;
    double                  bitrate;
    rational_t              framerate;
    rational_t              timebase;
    uint32_t                start_dts_offset;
    uint8_t                *extra_data;
    size_t                  extra_size;
};

/**
 * This structure stores compressed data.
 */
struct audio_packet {
    uint8_t             *data;
    size_t               size;
    uint64_t             pts;
    uint64_t             dts;
    int                  track_idx;
    struct audio_encoder encoder;
};

struct audio_packet *audio_packet_create(void *data, size_t len);
void audio_packet_destroy(struct audio_packet *packet);

#ifdef __cplusplus
}
#endif
#endif
