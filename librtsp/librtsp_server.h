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
