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
#include "libavcap.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#if defined (OS_LINUX)
extern struct avcap_ops dummy_ops;
extern struct avcap_ops v4l2_ops;
extern struct avcap_ops uvc_ops;
extern struct avcap_ops pulseaudio_ops;
extern struct avcap_ops xcbgrab_ops;
#elif defined (OS_WINDOWS)
extern struct avcap_ops dshow_ops;
#elif defined (OS_RTOS)
extern struct avcap_ops esp32cam_ops;
#endif

struct avcap_backend {
    enum avcap_backend_type type;
    const struct avcap_ops *ops;
};

static struct avcap_backend avcap_list[] = {
#if defined (OS_LINUX)
    {AVCAP_BACKEND_DUMMY,      &dummy_ops},
    {AVCAP_BACKEND_V4L2,       &v4l2_ops},
    {AVCAP_BACKEND_UVC,        &uvc_ops},
    {AVCAP_BACKEND_PULSEAUDIO, &pulseaudio_ops},
    {AVCAP_BACKEND_XCB,        &xcbgrab_ops},
#elif defined (OS_WINDOWS)
    {AVCAP_BACKEND_DSHOW,      &dshow_ops},
#elif defined (OS_RTOS)
    {AVCAP_BACKEND_ESP32CAM,   &esp32cam_ops},
#endif
};

struct avcap_ctx *avcap_open(const char *dev, struct avcap_config *conf)
{
    enum avcap_backend_type backend;
    struct avcap_ctx *avcap;
    backend = conf->backend;

    if (!conf) {
        printf("%s:%d invalid paraments!\n", __func__, __LINE__);
        return NULL;
    }
    avcap = (struct avcap_ctx *)calloc(1, sizeof(struct avcap_ctx));
    if (!avcap) {
        printf("malloc failed!\n");
        return NULL;
    }
    avcap->ops = avcap_list[backend].ops;
    if (!avcap->ops) {
        printf("avcap->ops %d is NULL!\n", backend);
        return NULL;
    }
    avcap->opaque = avcap->ops->_open(avcap, dev, conf);
    if (!avcap->opaque) {
        printf("open %s failed!\n", dev);
        goto failed;
    }
    return avcap;
failed:
    free(avcap);
    return NULL;
}

void avcap_close(struct avcap_ctx *avcap)
{
    if (!avcap) {
        printf("%s:%d invalid paraments!\n", __func__, __LINE__);
        return;
    }
    avcap->ops->_close(avcap);
    free(avcap);
}

int avcap_ioctl(struct avcap_ctx *avcap, unsigned long int cmd, ...)
{
    int ret;
    void *arg;
    va_list ap;
    va_start(ap, cmd);
    arg = va_arg(ap, void *);
    va_end(ap);

    switch (cmd) {
    case VIDCAP_GET_CAP:
        ret = avcap->ops->ioctl(avcap, cmd, NULL);
        break;
    case VIDCAP_SET_CTRL: {
        struct video_ctrl *vctrl;
        vctrl = (struct video_ctrl *)arg;
        ret = avcap->ops->ioctl(avcap, vctrl->cmd, vctrl->val);
        } break;
    case VIDCAP_SET_CONF:
        ret = avcap->ops->ioctl(avcap, cmd, (struct avcap_config *)arg);
        break;
    default:
        ret = avcap->ops->ioctl(avcap, cmd, arg);
        break;
    }
    return ret;
}

int avcap_query_frame(struct avcap_ctx *avcap, struct media_frame *frame)
{
    if (!avcap || !frame) {
        printf("%s:%d invalid paraments!\n", __func__, __LINE__);
        return -1;
    }
    return avcap->ops->query_frame(avcap, frame);
}

int avcap_start_stream(struct avcap_ctx *avcap, media_frame_cb *cb)
{
    if (!avcap) {
        printf("%s:%d invalid paraments!\n", __func__, __LINE__);
        return -1;
    }
    avcap->on_media_frame = cb;
    return avcap->ops->start_stream(avcap);
}

int avcap_stop_stream(struct avcap_ctx *avcap)
{
    if (!avcap) {
        printf("%s:%d invalid paraments!\n", __func__, __LINE__);
        return -1;
    }
    return avcap->ops->stop_stream(avcap);
}


