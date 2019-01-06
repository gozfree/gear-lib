/******************************************************************************
 * Copyright (C) 2014-2018 Zhifeng Gong <gozfree@163.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with libraries; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
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
