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
#include <gear-lib/libthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdarg.h>
#include <sys/uio.h>
#include <sys/mman.h>
#include <sys/epoll.h>
#include <pulse/pulseaudio.h>

struct pulse_ctx {
    int fd;
    struct uac_ctx *parent;
    bool is_streaming;
    pa_threaded_mainloop *mainloop;
    pa_context           *ctx;
    pa_context_state_t    state;
};

#define timeval2ns(tv) \
    (((uint64_t)tv.tv_sec * 1000000000) + ((uint64_t)tv.tv_usec * 1000))


static void on_pulse_state(pa_context *ctx, void *userdata)
{
    struct pulse_ctx *c = userdata;

    if (c->ctx != ctx) {
        printf("c->ctx=%p, ctx=%p\n", c->ctx, ctx);
        return;
    }

    switch (pa_context_get_state(ctx)) {
        case PA_CONTEXT_UNCONNECTED:
            printf("%s:%d PA_CONTEXT_UNCONNECTED\n", __func__, __LINE__);
            break;
        case PA_CONTEXT_CONNECTING:
            printf("%s:%d PA_CONTEXT_CONNECTING\n", __func__, __LINE__);
            break;
        case PA_CONTEXT_AUTHORIZING:
            printf("%s:%d PA_CONTEXT_AUTHORIZING\n", __func__, __LINE__);
            break;
        case PA_CONTEXT_SETTING_NAME:
            printf("%s:%d PA_CONTEXT_SETTING_NAME\n", __func__, __LINE__);
            break;
        case PA_CONTEXT_READY:
            printf("%s:%d PA_CONTEXT_READY\n", __func__, __LINE__);
            break;
        case PA_CONTEXT_FAILED:
            printf("%s:%d PA_CONTEXT_FAILED\n", __func__, __LINE__);
            break;
        case PA_CONTEXT_TERMINATED:
            printf("%s:%d PA_CONTEXT_TERMINATED\n", __func__, __LINE__);
            break;
        default:
            printf("%s:%d xxxx\n", __func__, __LINE__);
            break;
    }
    pa_threaded_mainloop_signal(c->mainloop, 0);
}

static void pa_server_info_cb(pa_context *ctx, const pa_server_info *info, void *data)
{
    struct pulse_ctx *c = data;

    if (c->ctx != ctx) {
        printf("c->ctx=%p, ctx=%p\n", c->ctx, ctx);
        return;
    }
    printf("========pulse audio information========\n");
    printf("      Server Version: %s\n",   info->server_version);
    printf("         Server Name: %s\n",   info->server_name);
    printf(" Default Source Name: %s\n",   info->default_source_name);
    printf("   Default Sink Name: %s\n",   info->default_sink_name);
    printf("           Host Name: %s\n",   info->host_name);
    printf("           User Name: %s\n",   info->user_name);
    printf("            Channels: %hhu\n", info->sample_spec.channels);
    printf("                Rate: %u\n",   info->sample_spec.rate);
    printf("          Frame Size: %lu\n",  pa_frame_size(&info->sample_spec));
    printf("         Sample Size: %lu\n",  pa_sample_size(&info->sample_spec));
    printf(" ChannelMap Channels: %hhu\n", info->channel_map.channels);

    pa_threaded_mainloop_signal(c->mainloop, 0);
}

static void *uac_pa_open(struct uac_ctx *uac, const char *dev, struct uac_config *conf)
{
    pa_mainloop_api *mainloop_api;
    struct pulse_ctx *c = calloc(1, sizeof(struct pulse_ctx));

    c->mainloop = pa_threaded_mainloop_new();
    if (!c->mainloop) {
        printf("pa_threaded_mainloop_new failed!\n");
    }
    mainloop_api = pa_threaded_mainloop_get_api(c->mainloop);
    if (!mainloop_api) {
        printf("pa_threaded_mainloop_get_api failed!\n");
    }

    c->ctx = pa_context_new(mainloop_api, "libuac");
    if (!c->ctx) {
        printf("pa_context_new failed!\n");
    }

    pa_context_set_state_callback(c->ctx, on_pulse_state, c);
    pa_context_connect(c->ctx, NULL, PA_CONTEXT_NOFLAGS, NULL);
    pa_threaded_mainloop_start(c->mainloop);

    pa_threaded_mainloop_lock(c->mainloop);
    while ((c->state = pa_context_get_state(c->ctx)) != PA_CONTEXT_READY) {
        if (c->state == PA_CONTEXT_FAILED || c->state == PA_CONTEXT_TERMINATED) {
            printf("pa_context_state is invalid!\n");
            break;
        }
        pa_threaded_mainloop_wait(c->mainloop);
    }
    pa_threaded_mainloop_unlock(c->mainloop);
    pa_context_set_state_callback(c->ctx, NULL, c);


    pa_operation *op = NULL;
    pa_operation_state_t op_state;

    pa_threaded_mainloop_lock(c->mainloop);
    op = pa_context_get_server_info(c->ctx, pa_server_info_cb, c);
    while ((op_state = pa_operation_get_state(op)) != PA_OPERATION_DONE) {
        if (op_state == PA_OPERATION_CANCELLED) {
            printf("Get server info operation cancelled!");
            break;
        }
        printf("%s:%d xxxx\n", __func__, __LINE__);
        pa_threaded_mainloop_wait(c->mainloop);
      }
    pa_operation_unref(op);
    pa_threaded_mainloop_unlock(c->mainloop);

    c->parent = uac;

    return c;
}

static int uac_pa_start_stream(struct uac_ctx *uac)
{
    return 0;
}

static int uac_pa_stop_stream(struct uac_ctx *uac)
{
   return 0;
}

static int uac_pa_query_frame(struct uac_ctx *uac, struct audio_frame *frame)
{
    return 0;
}

static void uac_pa_close(struct uac_ctx *uac)
{
}

struct uac_ops pa_ops = {
    .open         = uac_pa_open,
    .close        = uac_pa_close,
    .ioctl        = NULL,
    .start_stream = uac_pa_start_stream,
    .stop_stream  = uac_pa_stop_stream,
    .query_frame  = uac_pa_query_frame,
};
