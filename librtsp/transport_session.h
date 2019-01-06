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
#ifndef TRANSPORT_SESSION_H
#define TRANSPORT_SESSION_H

#include "media_source.h"
#include "rtsp_parser.h"
#include "rtp.h"
#include <libthread.h>
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
