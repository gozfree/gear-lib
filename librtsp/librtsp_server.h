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
#ifndef LIBRTSP_SERVER_H
#define LIBRTSP_SERVER_H

#include <libskt.h>
#include <libdict.h>

#ifdef __cplusplus
extern "C" {
#endif

struct rtsp_server_ctx {
    int listen_fd;
    struct skt_addr host;
    struct gevent_base *evbase;
    void *connect_pool;
    void *transport_session_pool;
    void *media_source_pool;
    struct protocol_ctx *rtp_ctx;
    struct thread *master_thread;
    struct thread *worker_thread;
};

struct rtsp_server_ctx *rtsp_server_init(const char *host, uint16_t port);
void rtsp_deinit(struct rtsp_server_ctx *ctx);

#ifdef __cplusplus
}
#endif
#endif
