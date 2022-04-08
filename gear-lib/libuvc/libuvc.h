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

#include <libposix.h>
#include <libmedia-io.h>
#include <stdio.h>
#include <stdint.h>
#if defined (OS_LINUX)
#define __USE_LINUX_IOCTL_DEFS
#include <sys/ioctl.h>
#endif

#define LIBUVC_VERSION "0.2.1"

#ifdef __cplusplus
extern "C" {
#endif

enum uvc_type {
#if defined (OS_LINUX)
    UVC_TYPE_DUMMY = 0,
    UVC_TYPE_V4L2,
#elif defined (OS_WINDOWS)
    UVC_TYPE_DSHOW,
#endif
    UVC_TYPE_MAX,
};

struct uvc_ctx;
struct uvc_ops;
typedef int (video_frame_cb)(struct uvc_ctx *c, struct video_frame *frame);

struct uvc_config {
    uint32_t width;
    uint32_t height;
    rational_t fps;
    enum pixel_format format;
};

struct uvc_image_quality {
    int brightness;
    int contrast;
    int saturation;
    int hue;
    int awb;
    int gamma;
    int sharpness;
};

struct uvc_ctx {
    int fd;
    struct uvc_config conf;
    struct uvc_ops *ops;
    video_frame_cb *on_video_frame;
    void *opaque;
};

struct video_cap {
    uint8_t desc[32];
    uint32_t version;
};

struct video_ctrl {
    uint32_t cmd;
    uint32_t val;
};

#define UVC_GET_CAP         _IOWR('V',  0, struct video_cap)
#define UVC_SET_CTRL        _IOWR('V',  1, struct video_ctrl)
#define UVC_SET_CONF        _IOWR('V',  2, struct uvc_config)
#define UVC_GET_LUMA        _IOWR('V',  3, int *)
#define UVC_SET_LUMA        _IOWR('V',  4, int)
#define UVC_GET_CTRST       _IOWR('V',  5, int *)
#define UVC_SET_CTRST       _IOWR('V',  6, int)
#define UVC_GET_SAT         _IOWR('V',  7, int *)
#define UVC_SET_SAT         _IOWR('V',  8, int)
#define UVC_GET_HUE         _IOWR('V',  9, int *)
#define UVC_SET_HUE         _IOWR('V', 10, int)
#define UVC_GET_AWB         _IOWR('V', 11, int *)
#define UVC_SET_AWB         _IOWR('V', 12, int)
#define UVC_GET_GAMMA       _IOWR('V', 13, int *)
#define UVC_SET_GAMMA       _IOWR('V', 14, int)
#define UVC_GET_SHARP       _IOWR('V', 15, int *)
#define UVC_SET_SHARP       _IOWR('V', 16, int)



struct uvc_ops {
    void *(*_open)(struct uvc_ctx *uvc, const char *dev, struct uvc_config *conf);
    void (*_close)(struct uvc_ctx *c);
    int (*ioctl)(struct uvc_ctx *c, unsigned long int cmd, ...);
    int (*start_stream)(struct uvc_ctx *c);
    int (*stop_stream)(struct uvc_ctx *c);
    int (*query_frame)(struct uvc_ctx *c, struct video_frame *frame);
};

GEAR_API struct uvc_ctx *uvc_open(enum uvc_type type, const char *dev, struct uvc_config *conf);
GEAR_API int uvc_ioctl(struct uvc_ctx *c, unsigned long int cmd, ...);
GEAR_API void uvc_close(struct uvc_ctx *c);

/*
 * active query frame one by one
 */
GEAR_API int uvc_query_frame(struct uvc_ctx *c, struct video_frame *frame);

/*
 * passive get frame when cb is set, otherwise need query frame one by one
 */
GEAR_API int uvc_start_stream(struct uvc_ctx *uvc, video_frame_cb *cb);
GEAR_API int uvc_stop_stream(struct uvc_ctx *uvc);

#ifdef __cplusplus
}
#endif
#endif
