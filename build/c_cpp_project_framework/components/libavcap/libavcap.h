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
#ifndef LIBAVCAP_H
#define LIBAVCAP_H

#include <libposix.h>
#include <libmedia-io.h>
#include <stdio.h>
#include <stdint.h>
#if defined (OS_LINUX)
#define __USE_LINUX_IOCTL_DEFS
#include <sys/ioctl.h>
#endif

#define LIBAVCAP_VERSION "0.0.1"

#ifdef __cplusplus
extern "C" {
#endif

enum avcap_type {
    AVCAP_TYPE_AUDIO,
    AVCAP_TYPE_VIDEO,
    AVCAP__TYPE_MAX
};

enum avcap_backend_type {
#if defined (OS_LINUX)
    AVCAP_BACKEND_DUMMY,
    AVCAP_BACKEND_V4L2,
    AVCAP_BACKEND_UVC,
    AVCAP_BACKEND_PULSEAUDIO,
    AVCAP_BACKEND_XCB,
#elif defined (OS_WINDOWS)
    AVCAP_BACKEND_DSHOW,
#elif defined (OS_RTOS)
    AVCAP_BACKEND_ESP32CAM,
#endif
    AVCAP_BACKEND_MAX,
};


#include "audiocap.h"
#include "videocap.h"

struct avcap_ctx;
typedef int (media_frame_cb)(struct avcap_ctx *c, struct media_frame *frame);

struct avcap_config {
    enum avcap_type type;
    enum avcap_backend_type backend;
    union {
        struct audiocap_config audio;
        struct videocap_config video;
    };
};

struct avcap_ctx {
    int fd;
    struct avcap_config conf;
    const struct avcap_ops *ops;
    media_frame_cb *on_media_frame;
    void *opaque;
};

struct avcap_ops {
    void *(*_open)(struct avcap_ctx *avcap, const char *dev, struct avcap_config *conf);
    void (*_close)(struct avcap_ctx *c);
    int (*ioctl)(struct avcap_ctx *c, unsigned long int cmd, ...);
    int (*start_stream)(struct avcap_ctx *c);
    int (*stop_stream)(struct avcap_ctx *c);
    int (*query_frame)(struct avcap_ctx *c, struct media_frame *frame);
};

#if 0
#define avcap_call(ctx, fn, args...)               \
    ({                                             \
        int __res;                                 \
        if (!(ctx) || !((ctx)->ops->fn))           \
            __res = -1;                            \
        else                                       \
            __res = (ctx)->ops->fn((ctx), ##args); \
            __res;                                 \
    })
#endif

GEAR_API struct avcap_ctx *avcap_open(const char *dev, struct avcap_config *conf);
GEAR_API int avcap_ioctl(struct avcap_ctx *c, unsigned long int cmd, ...);
GEAR_API void avcap_close(struct avcap_ctx *c);

/*
 * active query frame one by one
 */
GEAR_API int avcap_query_frame(struct avcap_ctx *c, struct media_frame *frame);

/*
 * passive get frame when cb is set, otherwise need query frame one by one
 */
GEAR_API int avcap_start_stream(struct avcap_ctx *avcap, media_frame_cb *cb);
GEAR_API int avcap_stop_stream(struct avcap_ctx *avcap);


#ifdef __cplusplus
}
#endif
#endif
