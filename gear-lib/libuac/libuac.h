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
#ifndef LIBUAC_H
#define LIBUAC_H

#include <gear-lib/libposix.h>
#include <gear-lib/libmedia-io.h>
#include <stdio.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct uac_ctx;
struct uac_ops;
typedef int (audio_frame_cb)(struct uac_ctx *c, struct audio_frame *frame);

struct uac_config {
    enum sample_format format;
    uint32_t           sample_rate;
    uint8_t            channels;
    const char        *device;
};

struct uac_ctx {
    int fd;
    struct uac_config conf;
    struct uac_ops *ops;
    audio_frame_cb *on_audio_frame;
    void *opaque;
};

struct uac_ops {
    void *(*open)(struct uac_ctx *uac, const char *dev, struct uac_config *conf);
    void (*close)(struct uac_ctx *c);
    int (*ioctl)(struct uac_ctx *c, unsigned long int cmd, ...);
    int (*start_stream)(struct uac_ctx *c);
    int (*stop_stream)(struct uac_ctx *c);
    int (*query_frame)(struct uac_ctx *c, struct audio_frame *frame);
};

struct uac_ctx *uac_open(const char *dev, struct uac_config *conf);
int uac_ioctl(struct uac_ctx *c, unsigned long int cmd, ...);
void uac_close(struct uac_ctx *c);

/*
 * active query frame one by one
 */
int uac_query_frame(struct uac_ctx *c, struct audio_frame *frame);

/*
 * passive get frame when cb is set, otherwise need query frame one by one
 */
int uac_start_stream(struct uac_ctx *uac, audio_frame_cb *cb);
int uac_stop_stream(struct uac_ctx *uac);




#ifdef __cplusplus
}
#endif
#endif
