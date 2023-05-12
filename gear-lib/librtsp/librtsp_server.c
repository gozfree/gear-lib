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
#include <libposix.h>
#include <liblog.h>
#include <libdict.h>
#include "librtsp.h"
#include <libsock.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>
#include <sys/time.h>

#include "media_source.h"
#include "transport_session.h"
#include "rtsp_parser.h"
#include "request_handle.h"

#define LOCAL_HOST          ((const char *)"127.0.0.1")

#define RTSP_REQUEST_LEN_MAX	(1024)

static void rtsp_connect_create(struct rtsp_server *rtsp, int fd, uint32_t ip, uint16_t port);
static void rtsp_connect_destroy(struct rtsp_server *rtsp, int fd);

static void on_recv(int fd, void *arg)
{
    int rlen, res;
    struct rtsp_request *req = (struct rtsp_request *)arg;
    memset(req->raw->iov_base, 0, RTSP_REQUEST_LEN_MAX);
    rlen = sock_recv(fd, req->raw->iov_base, RTSP_REQUEST_LEN_MAX);
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
        strcpy(req->cmd, "teardown");
        handle_rtsp_request(req);
        rtsp_connect_destroy(req->rtsp_server, fd);
    } else {
        loge("something error\n");
    }
}

static void on_error(int fd, void *arg)
{
    loge("error: %d\n", errno);
}

static void rtsp_connect_create(struct rtsp_server *rtsp, int fd, uint32_t ip, uint16_t port)
{
    char key[9];
    struct rtsp_request *req = calloc(1, sizeof(struct rtsp_request));
    if (!req) {
        loge("calloc failed!\n");
        return;
    }
    req->fd = fd;
    req->client.ip = ip;
    req->client.port = port;
    req->rtsp_server = rtsp;
    req->raw = iovec_create(RTSP_REQUEST_LEN_MAX);
    sock_set_noblk(fd, 1);
    req->event = gevent_create(fd, on_recv, NULL, on_error, req);
    if (-1 == gevent_add(rtsp->evbase, &req->event)) {
        loge("event_add failed!\n");
    }
    req->transport.fd = fd;
    snprintf(key, sizeof(key), "%d", fd);
    dict_add(rtsp->connect_pool, key, (char *)req);
    logi("fd = %d, req=%p\n", fd, req);
}

static void rtsp_connect_destroy(struct rtsp_server *rtsp, int fd)
{
    char key[9];
    struct rtsp_request *req;
    snprintf(key, sizeof(key), "%d", fd);
    req = (struct rtsp_request *)dict_get(rtsp->connect_pool, key, NULL);
    logi("fd = %d, req=%p\n", fd, req);
    dict_del(rtsp->connect_pool, key);
    gevent_del(rtsp->evbase, &req->event);
    gevent_destroy(req->event);
    iovec_destroy(req->raw);
    sock_close(fd);
    free(req);
}

static void on_connect(int fd, void *arg)
{
    int afd;
    uint32_t ip;
    uint16_t port;
    struct rtsp_server *rtsp = (struct rtsp_server *)arg;

    afd = sock_accept(fd, &ip, &port);
    if (afd == -1) {
        loge("sock_accept failed: %d\n", errno);
        return;
    }
    logd("connect fd = %d, accept fd = %d\n", fd, afd);
    rtsp_connect_create(rtsp, afd, ip, port);
}

static void *rtsp_thread_event(struct thread *t, void *arg)
{
    struct rtsp_server *rc = (struct rtsp_server *)arg;
    gevent_base_loop(rc->evbase);
    return NULL;
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

static int master_thread_create(struct rtsp_server *c)
{
    int fd = sock_tcp_bind_listen(c->host.ip_str, c->host.port);
    if (fd == -1) {
        goto failed;
    }
    media_source_register_all();
    c->transport_session_pool = transport_session_pool_create();
    c->connect_pool = connect_pool_create();
    c->listen_fd = fd;
    c->evbase = gevent_base_create();
    if (!c->evbase) {
        goto failed;
    }
    c->ev_connect = gevent_create(fd, on_connect, NULL, on_error, (void *)c);
    if (-1 == gevent_add(c->evbase, &c->ev_connect)) {
        loge("event_add failed!\n");
        gevent_destroy(c->ev_connect);
    }
    c->master_thread = thread_create(rtsp_thread_event, c);
    if (!c->master_thread) {
        loge("thread_create failed!\n");
        goto failed;
    }
    return 0;

failed:
    if (c->ev_connect) {
        gevent_destroy(c->ev_connect);
    }
    if (fd != -1) {
        sock_close(fd);
    }
    return -1;
}

static void master_thread_destroy(struct rtsp_server *c)
{
    gevent_del(c->evbase, &c->ev_connect);
    gevent_base_loop_break(c->evbase);
    thread_join(c->master_thread);
    thread_destroy(c->master_thread);
    gevent_base_destroy(c->evbase);
    connect_pool_destroy(c->connect_pool);
    transport_session_pool_destroy(c->transport_session_pool);
}

struct rtsp_server *rtsp_server_init(const char *ip, uint16_t port)
{
    struct rtsp_server *c = calloc(1, sizeof(struct rtsp_server));
    if (!c) {
        loge("malloc rtsp_server failed!\n");
        return NULL;
    }
    if (ip) {
        strcpy(c->host.ip_str, ip);
    }
    c->host.port = port;
    return c;
}

int rtsp_server_dispatch(struct rtsp_server *c)
{
    return master_thread_create(c);
}

void rtsp_server_deinit(struct rtsp_server *c)
{
    master_thread_destroy(c);
    free(c);
}
