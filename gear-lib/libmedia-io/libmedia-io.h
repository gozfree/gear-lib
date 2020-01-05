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

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "audio-def.h"
#include "video-def.h"

/**
 * This structure describes decoded (raw) data.
 */
struct media_frame {
    union {
        struct audio_frame audio;
        struct video_frame video;
    };
};

/**
 * This structure stores compressed data.
 */

enum media_packet_type {
    MEDIA_PACKET_AUDIO,
    MEDIA_PACKET_VIDEO
};

struct media_packet {
    union {
        struct audio_packet *audio;
        struct video_packet *video;
    };
    enum media_packet_type type;
    void *opaque;
};

struct media_packet *media_packet_create(enum media_packet_type type,
                                            void *data, size_t len);
void media_packet_destroy(struct media_packet *packet);

#ifdef __cplusplus
}
#endif
#endif
