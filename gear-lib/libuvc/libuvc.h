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
#ifndef LIBUVC_H
#define LIBUVC_H

#include <stdio.h>
#include <stdint.h>
#if defined (__linux__) || defined (__CYGWIN__)
#define __USE_LINUX_IOCTL_DEFS
#include <sys/ioctl.h>
#elif defined (__WIN32__) || defined (WIN32) || defined (_MSC_VER)
#include "libposix4win.h"
#endif

#include <gear-lib/libmedia-io.h>

#ifdef __cplusplus
extern "C" {
#endif

struct uvc_ctx;
struct uvc_ops;
typedef int (*on_stream_data)(struct uvc_ctx *c, void *data, size_t len);

struct uvc_ctx {
    int fd;
    uint32_t width;
    uint32_t height;
    uint32_t fps_num;
    uint32_t fps_den;
    enum video_format format;
    struct uvc_ops *ops;
    void *opaque;
    on_stream_data *on_data;
};

struct video_cap {
    uint8_t desc[32];
    uint32_t version;
};

struct video_ctrl {
    uint32_t cmd;
    uint32_t val;
};

#define UVC_GET_CAP  _IOWR('V',  0, struct video_cap)
#define UVC_SET_CTRL _IOWR('V',  1, struct video_ctrl)

struct uvc_ops {
    void *(*open)(struct uvc_ctx *uvc, const char *dev, uint32_t width, uint32_t height);
    void (*close)(struct uvc_ctx *c);
    int (*dequeue)(struct uvc_ctx *c, struct video_frame *frame);
    int (*enqueue)(struct uvc_ctx *c, void *buf, size_t len);
    int (*ioctl)(struct uvc_ctx *c, unsigned long int cmd, ...);
    int (*print_info)(struct uvc_ctx *c);
    int (*start_stream)(struct uvc_ctx *c);
    int (*stop_stream)(struct uvc_ctx *c);
};

struct uvc_ctx *uvc_open(const char *dev, uint32_t width, uint32_t height);
int uvc_ioctl(struct uvc_ctx *c, unsigned long int cmd, ...);
void uvc_close(struct uvc_ctx *c);

int uvc_start_stream(struct uvc_ctx *uvc, on_stream_data *strm_cb);
int uvc_stop_stream(struct uvc_ctx *uvc);
int uvc_query_frame(struct uvc_ctx *c, struct video_frame *frame);

#ifdef __cplusplus
}
#endif
#endif
