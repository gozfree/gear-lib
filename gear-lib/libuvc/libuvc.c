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
#include "libuvc.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

#if defined (__linux__) || defined (__CYGWIN__)
extern struct uvc_ops v4l2_ops;
#elif defined (__WIN32__) || defined (WIN32) || defined (_MSC_VER)
extern struct uvc_ops dshow_ops;
#endif

static struct uvc_ops *uvc_ops[] = {
#if defined (__linux__) || defined (__CYGWIN__)
    &v4l2_ops,
#elif defined (__WIN32__) || defined (WIN32) || defined (_MSC_VER)
    &dshow_ops,
#endif
    NULL
};

int uvc_print_info(struct uvc_ctx *c)
{
    return c->ops->print_info(c);
}

struct uvc_ctx *uvc_open(const char *dev, int width, int height)
{
    struct uvc_ctx *uvc = (struct uvc_ctx *)calloc(1, sizeof(struct uvc_ctx));
    if (!uvc) {
        printf("malloc failed!\n");
        return NULL;
    }
    uvc->ops = uvc_ops[0];
    uvc->opaque = uvc->ops->open(uvc, dev, width, height);
    if (!uvc->opaque) {
        printf("open %s failed!\n", dev);
        goto failed;
    }
    uvc->width = width;
    uvc->height = height;
    return uvc;
failed:
    free(uvc);
    return NULL;
}

int uvc_read(struct uvc_ctx *uvc, void *buf, size_t len)
{
    if (-1 == uvc->ops->write(uvc, NULL, 0)) {
        return -1;
    }
    return uvc->ops->read(uvc, buf, len);
}

int uvc_start_stream(struct uvc_ctx *uvc)
{
    return uvc->ops->start_stream(uvc);
}

int uvc_stop_stream(struct uvc_ctx *uvc)
{
    return uvc->ops->stop_stream(uvc);
}

int uvc_ioctl(struct uvc_ctx *uvc, uint32_t cmd, void *buf, int len)
{
    struct video_ctrl *vctrl;
    switch (cmd) {
    case UVC_GET_CAP:
        uvc->ops->print_info(uvc);
        break;
    case UVC_SET_CTRL:
        vctrl = (struct video_ctrl *)buf;
        uvc->ops->ioctl(uvc, vctrl->cmd, vctrl->val);
        break;
    default:
        printf("cmd %d not supported yet!\n", cmd);
        break;
    }
    return 0;
}

void uvc_close(struct uvc_ctx *uvc)
{
    if (!uvc) {
        return;
    }
    uvc->ops->close(uvc);
    free(uvc);
}
