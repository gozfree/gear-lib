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
#include "librtmpc.h"
#include "rtmp.h"
#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

void rtmpc_destroy(struct rtmpc *rtmpc)
{
    if (!rtmpc) {
        return;
    }
    RTMP_Close(rtmpc->base);
    RTMP_Free(rtmpc->base);
    rtmpc->base = NULL;
    queue_destroy(rtmpc->q);
    flv_mux_destroy(rtmpc->flv);
    free(rtmpc);
}

static void *item_alloc_hook(void *data, size_t len, void *arg)
{
    struct media_packet *new_pkt;
    struct media_packet *pkt = (struct media_packet *)arg;
    if (!pkt) {
        printf("calloc packet failed!\n");
        return NULL;
    }
    new_pkt = media_packet_copy(pkt, MEDIA_MEM_SHALLOW);
    return new_pkt;
}

static void item_free_hook(void *data)
{
    struct media_packet *pkt = (struct media_packet *)data;
    media_packet_destroy(pkt);
}

static int flv_mux_output(void *ctx, uint8_t *data, size_t size, int strm_idx)
{
    RTMP *base = ctx;
    //DUMP_BUFFER(data, size);
    return RTMP_Write(base, (const char *)data, size, strm_idx);
}

struct rtmpc *rtmpc_create(const char *url)
{
    RTMP *base;
    struct rtmpc *rtmpc = (struct rtmpc *)calloc(1, sizeof(struct rtmpc));
    if (!rtmpc) {
        printf("malloc rtmpc failed!\n");
        goto failed;
    }
    base = RTMP_Alloc();
    if (!base) {
        printf("RTMP_Alloc failed!\n");
        goto failed;
    }
    RTMP_Init(base);
    RTMP_LogSetLevel(RTMP_LOGINFO);

    if (!RTMP_SetupURL(base, (char *)url)) {
        printf("RTMP_SetupURL failed!\n");
        goto failed;
    }

    RTMP_EnableWrite(base);

    RTMP_AddStream(base, NULL);

    if (!RTMP_Connect(base, NULL)) {
        printf("RTMP_Connect failed!\n");
        goto failed;
    }
    if (!RTMP_ConnectStream(base, 0)){
        printf("RTMP_ConnectStream failed!\n");
        goto failed;
    }

    rtmpc->flv = flv_mux_create(flv_mux_output, base);

    rtmpc->q = queue_create();
    if (!rtmpc->q) {
        printf("queue_create failed!\n");
        goto failed;
    }
    queue_set_hook(rtmpc->q, item_alloc_hook, item_free_hook);
    rtmpc->base = base;
    rtmpc->is_run = false;
    rtmpc->is_start = false;
    return rtmpc;

failed:
    if (rtmpc) {
        if (rtmpc->q) {
            queue_destroy(rtmpc->q);
        }
        free(rtmpc);
    }
    return NULL;
}

static inline void set_rtmp_val(AVal *val, char *str, int len)
{
    val->av_val = str;
    val->av_len = len;
}

struct rtmpc *rtmpc_create2(struct rtmp_url *url)
{
    RTMP *base;
    struct rtmpc *rtmpc = (struct rtmpc *)calloc(1, sizeof(struct rtmpc));
    if (!rtmpc) {
        printf("malloc rtmpc failed!\n");
        goto failed;
    }
    base = RTMP_Alloc();
    if (!base) {
        printf("RTMP_Alloc failed!\n");
        goto failed;
    }
    RTMP_Init(base);
    RTMP_LogSetLevel(RTMP_LOGINFO);

    if (!RTMP_SetupURL(base, url->addr)) {
        printf("RTMP_SetupURL failed!\n");
        goto failed;
    }

    RTMP_EnableWrite(base);

    if (url->username) {
        set_rtmp_val(&base->Link.pubUser, url->username, strlen(url->username));
    }
    if (url->password) {
        set_rtmp_val(&base->Link.pubPasswd, url->password, strlen(url->password));
    }
	base->Link.swfUrl = base->Link.tcUrl;
    if (url->key) {
        RTMP_AddStream(base, url->key);
    }

    if (!RTMP_Connect(base, NULL)) {
        printf("RTMP_Connect failed!\n");
        goto failed;
    }
    if (!RTMP_ConnectStream(base, 0)){
        printf("RTMP_ConnectStream failed!\n");
        goto failed;
    }

    rtmpc->flv = flv_mux_create(flv_mux_output, base);

    rtmpc->q = queue_create();
    if (!rtmpc->q) {
        printf("queue_create failed!\n");
        goto failed;
    }
    queue_set_hook(rtmpc->q, item_alloc_hook, item_free_hook);
    rtmpc->base = base;
    rtmpc->is_run = false;
    rtmpc->is_start = false;
    return rtmpc;

failed:
    if (rtmpc) {
        if (rtmpc->q) {
            queue_destroy(rtmpc->q);
        }
        free(rtmpc);
    }
    return NULL;
}

int rtmpc_stream_add(struct rtmpc *rtmpc, struct media_packet *pkt)
{
    return flv_mux_add_media(rtmpc->flv, pkt);
}

int rtmpc_send_packet(struct rtmpc *rtmpc, struct media_packet *pkt)
{
    struct queue_item *item = NULL;
    if (!rtmpc || !pkt) {
        printf("%s invalid parament!\n", __func__);
        return -1;
    }
    switch (pkt->type) {
    case MEDIA_TYPE_AUDIO:
        item = queue_item_alloc(rtmpc->q, pkt->audio->data, pkt->audio->size, pkt);
        break;
    case MEDIA_TYPE_VIDEO:
        item = queue_item_alloc(rtmpc->q, pkt->video->data, pkt->video->size, pkt);
        break;
    default:
        break;
    }
    if (!item) {
        printf("item_alloc packet type %d failed!\n", pkt->type);
        return -1;
    }
    if (0 != queue_push(rtmpc->q, item)) {
        printf("queue_push failed!\n");
        return -1;
    }
    return 0;
}

static void *rtmpc_stream_thread(struct thread *t, void *arg)
{
    struct media_packet *pkt;
    struct rtmpc *rtmpc = (struct rtmpc *)arg;
    queue_flush(rtmpc->q);
    rtmpc->is_run = true;
    while (rtmpc->is_run) {
        struct queue_item *it = queue_pop(rtmpc->q);
        if (!it) {
            usleep(200000);
            continue;
        }
        pkt = (struct media_packet *)it->opaque.iov_base;
        flv_write_packet(rtmpc->flv, pkt);
        queue_item_free(rtmpc->q, it);
    }
    return NULL;
}

void rtmpc_stream_stop(struct rtmpc *rtmpc)
{
    rtmpc->is_run = false;
    if (rtmpc) {
        queue_flush(rtmpc->q);
        thread_join(rtmpc->thread);
        thread_destroy(rtmpc->thread);
        rtmpc->thread = NULL;
        rtmpc->is_start = false;
    }
}

int rtmpc_stream_start(struct rtmpc *rtmpc)
{
    if (!rtmpc) {
        return -1;
    }
    if (rtmpc->is_start) {
        printf("rtmpc stream already start!\n");
        return -1;
    }
    rtmpc->thread = thread_create(rtmpc_stream_thread, rtmpc);
    if (!rtmpc->thread) {
        rtmpc->is_start = false;
        printf("thread_create failed!\n");
        return -1;
    }
    thread_set_name(rtmpc->thread, "rtmpc_stream");
    rtmpc->is_start = true;
    return 0;
}
