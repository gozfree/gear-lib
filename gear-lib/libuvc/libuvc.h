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
#include <sys/uio.h>
#define __USE_LINUX_IOCTL_DEFS
#include <sys/ioctl.h>
#elif defined (__WIN32__) || defined (WIN32) || defined (_MSC_VER)
#include "libposix4win.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct uvc_ops;

struct uvc_ctx {
    int fd;
    int width;
    int height;
    struct timeval timestamp;
    struct uvc_ops *ops;
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

#define UVC_GET_CAP  _IOWR('V',  0, struct video_cap)
#define UVC_SET_CTRL _IOWR('V',  1, struct video_ctrl)

struct uvc_ops {
    void *(*open)(struct uvc_ctx *uvc, const char *dev, int width, int height);
    void (*close)(struct uvc_ctx *c);
    int (*read)(struct uvc_ctx *c, void *buf, size_t len);
    int (*write)(struct uvc_ctx *c, void *buf, size_t len);
    int (*ioctl)(struct uvc_ctx *c, uint32_t cid, int value);
    int (*print_info)(struct uvc_ctx *c);
    int (*start_stream)(struct uvc_ctx *c);
    int (*stop_stream)(struct uvc_ctx *c);
};


struct uvc_ctx *uvc_open(const char *dev, int width, int height);
int uvc_print_info(struct uvc_ctx *c);
int uvc_read(struct uvc_ctx *c, void *buf, size_t len);
int uvc_ioctl(struct uvc_ctx *c, uint32_t cmd, void *buf, int len);
void uvc_close(struct uvc_ctx *c);

int uvc_start_stream(struct uvc_ctx *uvc);
int uvc_stop_stream(struct uvc_ctx *uvc);

#ifdef __cplusplus
}
#endif
#endif
