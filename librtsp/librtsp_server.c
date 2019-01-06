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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>
#include <sys/time.h>
#include <libmacro.h>
#include <liblog.h>
#include <libgevent.h>
#include <libthread.h>
#include <libdict.h>
#include "librtsp.h"
#include "media_source.h"
#include "transport_session.h"
#include "rtsp_parser.h"
#include "request_handle.h"

#define LOCAL_HOST          ((const char *)"127.0.0.1")

#define RTSP_REQUEST_LEN_MAX	(1024)

static void rtsp_connect_create(struct rtsp_server_ctx *rtsp, int fd, uint32_t ip, uint16_t port);
static void rtsp_connect_destroy(struct rtsp_server_ctx *rtsp, int fd);

static void on_recv(int fd, void *arg)
{
    int rlen, res;
    struct rtsp_request *req = (struct rtsp_request *)arg;
    memset(req->raw->iov_base, 0, RTSP_REQUEST_LEN_MAX);
    rlen = skt_recv(fd, req->raw->iov_base, RTSP_REQUEST_LEN_MAX);
    if (rlen > 0) {
        req->raw->iov_len = rlen;
        res = parse_rtsp_request(req);
        if (res == -1) {
            loge("parse_rtsp_request failed\n");
            return;
        }
        res = handle_rtsp_request(req);
        if (res == -1) {
            loge("handle_rtsp_request failed\n");
            return;
        }
    } else if (rlen == 0) {
        loge("peer connect shutdown\n");
        rtsp_connect_destroy(req->rtsp_server_ctx, fd);
    } else {
        loge("something error\n");
    }
}

static void on_error(int fd, void *arg)
{
    loge("error: %d\n", errno);
}

static void rtsp_connect_create(struct rtsp_server_ctx *rtsp, int fd, uint32_t ip, uint16_t port)
{
    char key[9];
    struct rtsp_request *req = CALLOC(1, struct rtsp_request);
    req->fd = fd;
    req->client.ip = ip;
    req->client.port = port;
    req->rtsp_server_ctx = rtsp;
    req->raw = iovec_create(RTSP_REQUEST_LEN_MAX);
    skt_set_noblk(fd, 1);
    req->event = gevent_create(fd, on_recv, NULL, on_error, req);
    if (-1 == gevent_add(rtsp->evbase, req->event)) {
        loge("event_add failed!\n");
    }
    snprintf(key, sizeof(key), "%d", fd);
    dict_add(rtsp->connect_pool, key, (char *)req);
    logi("fd = %d, req=%p\n", fd, req);
}

static void rtsp_connect_destroy(struct rtsp_server_ctx *rtsp, int fd)
{
    char key[9];
    snprintf(key, sizeof(key), "%d", fd);
    struct rtsp_request *req = (struct rtsp_request *)dict_get(rtsp->connect_pool, key, NULL);
    logi("fd = %d, req=%p\n", fd, req);
    dict_del(rtsp->connect_pool, key);
    gevent_del(rtsp->evbase, req->event);
    gevent_destroy(req->event);
    iovec_destroy(req->raw);
    skt_close(fd);
    free(req);
}

static void on_connect(int fd, void *arg)
{
    int afd;
    uint32_t ip;
    uint16_t port;
    struct rtsp_server_ctx *rtsp = (struct rtsp_server_ctx *)arg;

    afd = skt_accept(fd, &ip, &port);
    if (afd == -1) {
        loge("skt_accept failed: %d\n", errno);
        return;
    }
    logd("connect fd = %d, accept fd = %d\n", fd, afd);
    rtsp_connect_create(rtsp, afd, ip, port);
}


static void *rtsp_thread_event(struct thread *t, void *arg)
{
    struct rtsp_server_ctx *rc = (struct rtsp_server_ctx *)arg;
    gevent_base_loop(rc->evbase);
    return NULL;
}

static void media_source_default(struct rtsp_server_ctx *c)
{
    //const char *name = "H264";
    const char *name = "uvc";
    logi("rtsp://%s:%d/%s\n", strlen(c->host.ip_str)?c->host.ip_str:LOCAL_HOST, c->host.port, name);
}

static void *connect_pool_create()
{
    return (void *)dict_new();
}

void connect_pool_destroy(void *pool)
{
    int rank = 0;
    char *key, *val;
    while (1) {
        rank = dict_enumerate((dict *)pool, rank, &key, &val);
        if (rank < 0) {
            break;
        }
        free(val);
    }
    dict_free((dict *)pool);
}


static int master_thread_create(struct rtsp_server_ctx *c)
{
    struct gevent *e = NULL;
    int fd = skt_tcp_bind_listen(c->host.ip_str, c->host.port);
    if (fd == -1) {
        goto failed;
    }
    media_source_register_all();
    c->transport_session_pool = transport_session_pool_create();
    c->connect_pool = connect_pool_create();
    media_source_default(c);
    c->listen_fd = fd;
    c->evbase = gevent_base_create();
    if (!c->evbase) {
        goto failed;
    }
    e = gevent_create(fd, on_connect, NULL, on_error, (void *)c);
    if (-1 == gevent_add(c->evbase, e)) {
        loge("event_add failed!\n");
        gevent_destroy(e);
    }
    c->master_thread = thread_create(rtsp_thread_event, c);
    if (!c->master_thread) {
        loge("thread_create failed!\n");
        goto failed;
    }
    return 0;

failed:
    if (e) {
        gevent_destroy(e);
    }
    if (fd != -1) {
        skt_close(fd);
    }
    return -1;
}

static void destroy_master_thread(struct rtsp_server_ctx *c)
{
    connect_pool_destroy(c->connect_pool);

}

struct rtsp_server_ctx *rtsp_server_init(const char *ip, uint16_t port)
{
    struct rtsp_server_ctx *c = CALLOC(1, struct rtsp_server_ctx);
    if (!c) {
        loge("malloc rtsp_server_ctx failed!\n");
        return NULL;
    }
    if (ip) {
        strcpy(c->host.ip_str, ip);
    }
    c->host.port = port;
    master_thread_create(c);
    return c;
}

void rtsp_server_deinit(struct rtsp_server_ctx *c)
{
    destroy_master_thread(c);
    free(c);
}
