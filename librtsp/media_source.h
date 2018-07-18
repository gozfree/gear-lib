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
#ifndef MEDIA_SOURCE_H
#define MEDIA_SOURCE_H
#include <stdint.h>
#include <stddef.h>
#include <sys/time.h>

#ifdef __cplusplus
extern "C" {
#endif

#define STREAM_NAME_LEN     (128)
#define DESCRIPTION_LEN     (128)
#define SDP_LEN_MAX 8192

typedef struct media_source {
    char name[STREAM_NAME_LEN];
    char info[DESCRIPTION_LEN];
    char sdp[SDP_LEN_MAX];
    struct timeval tm_create;
    int (*sdp_generate)(struct media_source *ms);
    int (*get_frame)();
} media_source_t;

void *media_source_pool_create();
void media_source_pool_destroy(void *pool);
struct media_source *media_source_new(void *pool, char *name, size_t size);
void media_source_del(void *pool, char *name);
struct media_source *media_source_lookup(void *pool, char *name);


typedef struct transport_session {
    uint32_t session_id;
    uint16_t rtp_port;
    uint16_t rtcp_port;
} transport_session_t;

void *transport_session_pool_create();
void transport_session_pool_destroy(void *pool);
struct transport_session *transport_session_new(void *pool);
void transport_session_del(void *pool, char *name);
struct transport_session *transport_session_lookup(void *pool, char *name);


#ifdef __cplusplus
}
#endif
#endif
