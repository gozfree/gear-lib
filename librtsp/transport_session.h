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

#include "rtsp_parser.h"
#include "librtp.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct transport_session {
    uint32_t session_id;
    struct rtp_socket *rtp_skt;
    //XXX
    void* rtp;
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

} transport_session_t;

void *transport_session_pool_create();
void transport_session_pool_destroy(void *pool);
struct transport_session *transport_session_new(void *pool, struct transport_header *transport);
void transport_session_del(void *pool, char *name);
struct transport_session *transport_session_lookup(void *pool, char *name);

#ifdef __cplusplus
}
#endif
#endif
