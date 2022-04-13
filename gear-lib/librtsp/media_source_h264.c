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
#include <liblog.h>
#include <libfile.h>
#include <libqueue.h>
#include <libmedia-io.h>
#include "sdp.h"
#include "media_source.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

struct h264_source_ctx {
    const char name[32];
    struct queue *q;
    int sps_cnt;
    int duration;
};

static void *item_alloc_hook(void *data, size_t len, void *arg)
{
    struct media_packet *new_pkt;
    struct media_packet *pkt = (struct media_packet *)arg;
    if (!pkt) {
        loge("calloc packet failed!\n");
        return NULL;
    }
    new_pkt = media_packet_copy(pkt, MEDIA_MEM_SHALLOW);
    logd("media_packet size=%d\n", media_packet_get_size(new_pkt));
    return new_pkt;
}

static void item_free_hook(void *data)
{
    struct media_packet *pkt = (struct media_packet *)data;
    media_packet_destroy(pkt);
}

static inline const uint8_t* h264_find_start_code(const uint8_t* ptr, const uint8_t* end)
{
    const uint8_t *p;
    for (p = ptr; p + 3 < end; p++) {
        if (0x00 == p[0] && 0x00 == p[1] && (0x01 == p[2] || (0x00==p[2] && 0x01==p[3]))) {
            return p;
        }
    }
    return end;
}

static int h264_parser_frame(struct h264_source_ctx *c, const char *name)
{
    int ret = 0;
    size_t count = 0;
    size_t bytes;
    const uint8_t *start, *end, *nalu, *nalu2;
    struct media_packet *pkt;
    struct queue_item *it;
    struct iovec *data = file_dump(name);
    if (!data) {
        loge("file_dump %s failed!\n", name);
        return -1;
    }
    start = data->iov_base;
    end = start + data->iov_len;
    nalu = h264_find_start_code(start, end);

    while (nalu < end) {
        nalu2 = h264_find_start_code(nalu + 4, end);
        bytes = nalu2 - nalu;
        pkt = media_packet_create(MEDIA_TYPE_VIDEO, MEDIA_MEM_SHALLOW, (uint8_t *)nalu, bytes);
        pkt->video->pts = 90000 * count++;
        pkt->video->dts = 90000 * count++;
        pkt->video->encoder.timebase.num = 30;
        pkt->video->encoder.timebase.den = 1;

        it = queue_item_alloc(c->q, pkt->video->data, pkt->video->size, pkt);
        if (!it) {
            loge("item_alloc packet type %d failed!\n", pkt->type);
            ret = -1;
            goto exit;
        }
        if (0 != queue_push(c->q, it)) {
            loge("queue_push failed!\n");
            ret = -1;
            goto exit;
        }
        nalu = nalu2;
    }
    c->duration = 40 * count;

exit:
    free(data->iov_base);
    return ret;
}

static int h264_file_open(struct media_source *ms, const char *name)
{
    struct h264_source_ctx *c = calloc(1, sizeof(struct h264_source_ctx));
    if (!c) {
        loge("calloc h264_source_ctx failed!\n");
        return -1;
    }

    c->q = queue_create();
    if (!c->q) {
        loge("queue_create failed!\n");
        return -1;
    }
    queue_set_hook(c->q, item_alloc_hook, item_free_hook);
    ms->opaque = c;
    h264_parser_frame(c, name);
    return 0;
}

static void h264_file_close(struct media_source *ms)
{
    struct h264_source_ctx *c = (struct h264_source_ctx *)ms->opaque;
    queue_destroy(c->q);
    free(c);
}

static int h264_file_read_frame(struct media_source *ms, void **data, size_t *len)
{
    struct h264_source_ctx *c = (struct h264_source_ctx *)ms->opaque;
    struct queue_item *it = queue_pop(c->q);
    *data = (struct media_packet *)it->opaque.iov_base;
    *len = it->opaque.iov_len;
    logd("queue_pop ptr=%p, data=%p, len=%d\n", it->opaque.iov_base, *data, it->opaque.iov_len);
    //item_free(c->q, it);
    return 0;
}

static int h264_file_write(struct media_source *ms, void *data, size_t len)
{
    struct h264_source_ctx *c = (struct h264_source_ctx *)ms->opaque;
    c = c;
    return 0;
}

static uint32_t get_random_number()
{
    struct timeval now = {0};
    gettimeofday(&now, NULL);
    srand(now.tv_usec);
    return (rand() % ((uint32_t)-1));
}

static int is_auth()
{
    return 0;
}

static int sdp_generate(struct media_source *ms)
{
    int n = 0;
    char p[SDP_LEN_MAX];
    uint32_t session_id = get_random_number();
    gettimeofday(&ms->tm_create, NULL);
    n += snprintf(p+n, sizeof(p)-n, "v=0\n");
    n += snprintf(p+n, sizeof(p)-n, "o=%s %"PRIu32" %"PRIu32" IN IP4 %s\n", is_auth()?"username":"-", session_id, 1, "0.0.0.0");
    n += snprintf(p+n, sizeof(p)-n, "s=%s\n", ms->name);
    n += snprintf(p+n, sizeof(p)-n, "i=%s\n", ms->info);
    n += snprintf(p+n, sizeof(p)-n, "c=IN IP4 0.0.0.0\n");
    n += snprintf(p+n, sizeof(p)-n, "t=0 0\n");
    n += snprintf(p+n, sizeof(p)-n, "a=range:npt=0-\n");
    n += snprintf(p+n, sizeof(p)-n, "a=sendonly\n");
    n += snprintf(p+n, sizeof(p)-n, "a=control:*\n");
    n += snprintf(p+n, sizeof(p)-n, "a=source-filter: incl IN IP4 * %s\r\n", "0.0.0.0");
    n += snprintf(p+n, sizeof(p)-n, "a=rtcp-unicast: reflection\r\n");
    n += snprintf(p+n, sizeof(p)-n, "a=x-qt-text-nam:%s\r\n", ms->name);
    n += snprintf(p+n, sizeof(p)-n, "a=x-qt-text-inf:%s\r\n", ms->info);

    n += snprintf(p+n, sizeof(p)-n, "m=video 0 RTP/AVP 96\r\n");
    n += snprintf(p+n, sizeof(p)-n, "a=rtpmap:96 H264/90000\r\n");
    n += snprintf(p+n, sizeof(p)-n, "a=fmtp:96 packetization-mode=1; profile-level-id=4D4028; sprop-parameter-sets=Z01AKJpkA8ARPy4C3AQEBQAAAwPoAADqYOhgBGMAAF9eC7y40MAIxgAAvrwXeXCg,aO44gA==;\r\n");
    n += snprintf(p+n, sizeof(p)-n, "a=cliprect:0,0,240,320\r\n");

    strcpy(ms->sdp, p);
    return 0;
}

struct media_source media_source_h264 = {
    .name         = "H264",
    .sdp_generate = sdp_generate,
    ._open         = h264_file_open,
    ._read         = h264_file_read_frame,
    ._write        = h264_file_write,
    ._close        = h264_file_close,
};
