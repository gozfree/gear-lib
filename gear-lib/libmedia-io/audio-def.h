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
    /* refer to pulseaudio and ffmpeg */
    SAMPLE_FORMAT_NONE,
    SAMPLE_FORMAT_PCM_U8,        /**< Unsigned 8 Bit PCM */
    SAMPLE_FORMAT_PCM_ALAW,      /**< 8 Bit a-Law */
    SAMPLE_FORMAT_PCM_ULAW,      /**< 8 Bit mu-Law */
    SAMPLE_FORMAT_PCM_S16LE,     /**< Signed 16 Bit PCM, little endian */
    SAMPLE_FORMAT_PCM_S16BE,     /**< Signed 16 Bit PCM, big endian */
    SAMPLE_FORMAT_PCM_S24LE,     /**< Signed 24 Bit PCM packed, little endian */
    SAMPLE_FORMAT_PCM_S24BE,     /**< Signed 24 Bit PCM packed, big endian */
    SAMPLE_FORMAT_PCM_S32LE,     /**< Signed 32 Bit PCM, little endian */
    SAMPLE_FORMAT_PCM_S32BE,     /**< Signed 32 Bit PCM, big endian */
    SAMPLE_FORMAT_PCM_S24_32LE,  /**< Signed 24 Bit PCM in LSB of 32 Bit words, little endian */
    SAMPLE_FORMAT_PCM_S24_32BE,  /**< Signed 24 Bit PCM in LSB of 32 Bit words, big endian */
    SAMPLE_FORMAT_PCM_F32LE,     /**< 32 Bit IEEE floating point, little endian, range -1.0 to 1.0 */
    SAMPLE_FORMAT_PCM_F32BE,     /**< 32 Bit IEEE floating point, big endian, range -1.0 to 1.0 */

    SAMPLE_FORMAT_PCM_S16LE_PLANAR,
    SAMPLE_FORMAT_PCM_S16BE_PLANAR,
    SAMPLE_FORMAT_PCM_S24LE_PLANAR,
    SAMPLE_FORMAT_PCM_S32LE_PLANAR,

    SAMPLE_FORMAT_PCM_MAX,       /**< Upper limit of valid sample types */
};

#ifndef AUDIO_MAX_CHANNELS
#define AUDIO_MAX_CHANNELS 8
#endif

/**
 * This structure describe attribute of audio producer, which is creator
 */
struct audio_producer {
    enum sample_format format;
    uint32_t           sample_rate;
    int                channels;
};

void audio_producer_dump(struct audio_producer *as);

struct audio_frame {
    uint8_t           *data[AUDIO_MAX_CHANNELS];
    uint32_t           frames;
    enum sample_format format;
    uint32_t           sample_rate;
    uint64_t           timestamp;//ns
    uint64_t           frame_id;
    uint64_t           total_size;
};

const char *sample_format_to_string(enum sample_format format);
enum sample_format sample_string_to_format(const char *name);

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
    media_mem_type_t     mem_type;
    uint64_t             pts;
    uint64_t             dts;
    int                  track_idx;
    struct audio_encoder encoder;
};

GEAR_API struct audio_packet *audio_packet_create(enum media_mem_type type, void *data, size_t len);
GEAR_API void audio_packet_destroy(struct audio_packet *packet);
GEAR_API struct audio_packet *audio_packet_copy(struct audio_packet *dst, const struct audio_packet *src, media_mem_type_t type);
GEAR_API void audio_encoder_dump(struct audio_encoder *ve);

#ifdef __cplusplus
}
#endif
#endif
