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
#ifndef LIBMEDIA_IO_H
#define LIBMEDIA_IO_H

#include <libposix.h>
#include <stdlib.h>

#define LIBMEDIA_IO_VERSION "0.2.0"

#ifdef __cplusplus
extern "C" {
#endif

enum media_type {
    MEDIA_TYPE_AUDIO,
    MEDIA_TYPE_VIDEO,
    MEDIA_TYPE_SUBTITLE,
    MEDIA_TYPE_ATTACHMENT,
    MEDIA_TYPE_DATA,
    MEDIA_TYPE_MAX
};


/*
 * define media frame or packet memory copy type:
 * MEDIA_MEM_DEEP: data point to the memory alloc by uplayer
 * MEDIA_MEM_SHALLOW: data point to the addr which from hardware or prev stage
 */
typedef enum media_mem_type {
    MEDIA_MEM_SHALLOW = 0,
    MEDIA_MEM_DEEP,
} media_mem_type_t;

#include "audio-def.h"
#include "video-def.h"
#include "video-conv.h"

/*
 * +--------------+
 * |media_producer|--(produce)-|
 * +--------------+            -> +-----------+ --(encode)-> +------------+ -- (mux) -> +------------+
 *                                |media_frame|              |media_packet|             |media_stream|
 * +--------------+            -- +-----------+ <-(decode)-- +------------+ <-(demux)-- +------------+
 * |media_consumer|<-(consume)-|
 * +--------------+
 */

struct media_producer {
    union {
        struct audio_producer audio;
        struct video_producer video;
    };
    enum media_type type;
};

GEAR_API void media_producer_dump_info(struct media_producer *mp);

/**
 * This structure describes decoded (raw) data.
 */
struct media_frame {
    union {
        struct audio_frame audio;
        struct video_frame video;
    };
    enum media_type type;
};


/**
 * This structure stores compressed data.
 */

struct media_packet {
    union {
        struct audio_packet *audio;
        struct video_packet *video;
    };
    enum media_type type;
};

GEAR_API struct media_packet *media_packet_create(enum media_type type, media_mem_type_t mem_type, void *data, size_t len);
GEAR_API void media_packet_destroy(struct media_packet *mp);
GEAR_API struct media_packet *media_packet_copy(const struct media_packet *src, media_mem_type_t type);
GEAR_API size_t media_packet_get_size(struct media_packet *mp);

struct media_encoder {
    union {
        struct audio_encoder audio;
        struct video_encoder video;
    };
    enum media_type type;
};

GEAR_API void media_encoder_dump_info(struct media_encoder *me);

#ifdef __cplusplus
}
#endif
#endif
