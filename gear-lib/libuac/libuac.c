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
#include "libuac.h"
#include <stdio.h>
#include <stdlib.h>

#if defined (OS_LINUX)
extern struct uac_ops pa_ops;
#elif defined (OS_WINDOWS)
#endif

static struct uac_ops *uac_ops[] = {
#if defined (OS_LINUX)
    &pa_ops,
#elif defined (OS_WINDOWS)
#endif
    NULL
};

struct uac_ctx *uac_open(const char *dev, struct uac_config *conf)
{
    struct uac_ctx *uac;
    if (!dev || !conf) {
        printf("invalid paraments!\n");
        return NULL;
    }
    uac = (struct uac_ctx *)calloc(1, sizeof(struct uac_ctx));
    if (!uac) {
        printf("malloc failed!\n");
        return NULL;
    }
    uac->ops = uac_ops[0];
    uac->opaque = uac->ops->open(uac, dev, conf);
    if (!uac->opaque) {
        printf("open %s failed!\n", dev);
        goto failed;
    }
    return uac;
failed:
    free(uac);
    return NULL;
}


void uac_close(struct uac_ctx *uac)
{
    if (!uac) {
        printf("invalid paraments!\n");
        return;
    }
    uac->ops->close(uac);
    free(uac);
}

int uac_query_frame(struct uac_ctx *uac, struct audio_frame *frame)
{
    if (!uac || !frame) {
        printf("invalid paraments!\n");
        return -1;
    }
    return uac->ops->query_frame(uac, frame);
}

int uac_start_stream(struct uac_ctx *uac, audio_frame_cb *cb)
{
    if (!uac) {
        printf("invalid paraments!\n");
        return -1;
    }
    uac->on_audio_frame = cb;
    return uac->ops->start_stream(uac);
}

int uac_stop_stream(struct uac_ctx *uac)
{
    if (!uac) {
        printf("invalid paraments!\n");
        return -1;
    }
    return uac->ops->stop_stream(uac);
}


