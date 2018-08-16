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

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

struct uvc_ctx {
    int fd;
    int width;
    int height;
    void *opaque;
};

struct uvc_ctx *uvc_open(const char *dev, int width, int height);
int uvc_print_info(struct uvc_ctx *c);
int uvc_read(struct uvc_ctx *c, void *buf, size_t len);
void uvc_close(struct uvc_ctx *c);


#ifdef __cplusplus
}
#endif
#endif
