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
#ifndef TRANSPORT_SESSION_H
#define TRANSPORT_SESSION_H

#include "media_source.h"
#include "rtsp_parser.h"
#include "rtp.h"
#include <gear-lib/libthread.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct transport_session {
    uint32_t session_id;
    struct rtp_context *rtp;
    struct gevent_base *evbase;
    //XXX
    int64_t dts_first; // first frame timestamp
    int64_t dts_last; // last frame timestamp
    uint64_t timestamp; // rtp timestamp
    uint64_t rtcp_clock;

    uint32_t ssrc;
    int bandwidth;
    int frequency;
    char name[64];
    int payload;
    void* packer; // rtp encoder
    uint8_t packet[1450];

    int track; // mp4 track
    struct thread *thread;
    struct thread *ev_thread;
    struct media_source *media_source;

} transport_session_t;

void *transport_session_pool_create();
void transport_session_pool_destroy(void *pool);
struct transport_session *transport_session_create(void *pool, struct transport_header *hdr);
void transport_session_destroy(void *pool, char *name);
struct transport_session *transport_session_lookup(void *pool, char *name);
int transport_session_start(struct transport_session *ts, struct media_source *ms);
int transport_session_pause(struct transport_session *s);
int transport_session_stop(struct transport_session *s);

#ifdef __cplusplus
}
#endif
#endif
