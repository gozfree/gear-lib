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
#include <sys/ioctl.h>
#include <sys/time.h>

#ifdef __cplusplus
extern "C" {
#endif

struct uvc_ctx {
    int fd;
    int width;
    int height;
    struct timeval timestamp;
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


struct uvc_ctx *uvc_open(const char *dev, int width, int height);
int uvc_print_info(struct uvc_ctx *c);
int uvc_read(struct uvc_ctx *c, void *buf, size_t len);
int uvc_ioctl(struct uvc_ctx *c, uint32_t cmd, void *buf, int len);
void uvc_close(struct uvc_ctx *c);


#ifdef __cplusplus
}
#endif
#endif
