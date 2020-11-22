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
#include <stdarg.h>

extern struct uvc_ops dummy_ops;
#if defined (OS_LINUX)
extern struct uvc_ops v4l2_ops;
#elif defined (OS_WINDOWS)
extern struct uvc_ops dshow_ops;
#endif

static struct uvc_ops *uvc_ops[] = {
    [UVC_TYPE_DUMMY] = &dummy_ops,
#if defined (OS_LINUX)
    [UVC_TYPE_V4L2]  = &v4l2_ops,
#elif defined (OS_WINDOWS)
    [UVC_TYPE_DSHOW] = &dshow_ops,
#endif
    [UVC_TYPE_MAX] = NULL,
};

struct uvc_ctx *uvc_open(enum uvc_type type, const char *dev, struct uvc_config *conf)
{
    struct uvc_ctx *uvc;
    if (!dev || !conf) {
        printf("invalid paraments!\n");
        return NULL;
    }
    uvc = (struct uvc_ctx *)calloc(1, sizeof(struct uvc_ctx));
    if (!uvc) {
        printf("malloc failed!\n");
        return NULL;
    }
    uvc->ops = uvc_ops[type];
    if (!uvc->ops) {
        printf("uvc->ops %d is NULL!\n", type);
        return NULL;
    }
    uvc->opaque = uvc->ops->open(uvc, dev, conf);
    if (!uvc->opaque) {
        printf("open %s failed!\n", dev);
        goto failed;
    }
    return uvc;
failed:
    free(uvc);
    return NULL;
}

int uvc_query_frame(struct uvc_ctx *uvc, struct video_frame *frame)
{
    if (!uvc || !frame) {
        printf("invalid paraments!\n");
        return -1;
    }
    return uvc->ops->query_frame(uvc, frame);
}

int uvc_start_stream(struct uvc_ctx *uvc, video_frame_cb *cb)
{
    if (!uvc) {
        printf("invalid paraments!\n");
        return -1;
    }
    uvc->on_video_frame = cb;
    return uvc->ops->start_stream(uvc);
}

int uvc_stop_stream(struct uvc_ctx *uvc)
{
    if (!uvc) {
        printf("invalid paraments!\n");
        return -1;
    }
    return uvc->ops->stop_stream(uvc);
}

int uvc_ioctl(struct uvc_ctx *uvc, unsigned long int cmd, ...)
{
    void *arg;
    va_list ap;
    va_start(ap, cmd);
    arg = va_arg(ap, void *);
    va_end(ap);

    switch (cmd) {
    case UVC_GET_CAP:
        uvc->ops->ioctl(uvc, cmd, NULL);
        break;
    case UVC_SET_CTRL: {
        struct video_ctrl *vctrl;
        vctrl = (struct video_ctrl *)arg;
        uvc->ops->ioctl(uvc, vctrl->cmd, vctrl->val);
    } break;
    default:
        printf("cmd %lu not supported yet!\n", cmd);
        break;
    }
    return 0;
}

void uvc_close(struct uvc_ctx *uvc)
{
    if (!uvc) {
        printf("invalid paraments!\n");
        return;
    }
    uvc->ops->close(uvc);
    free(uvc);
}
