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
#include "librpc.h"
#include <libgevent.h>
#include <libthread.h>
#include <libsock.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>

struct sk_ctx {
    int fd;
    struct sock_connection *connect;
};

static rpc_recv_cb *sk_recv_cb = NULL;

static void on_error(int fd, void *arg)
{
    printf("error: %d\n", errno);
}

static int sk_recv(struct rpc *rpc, void *buf, size_t len)
{
    int fd = 0;
    struct sk_ctx *c = (struct sk_ctx *)rpc->ctx;
    if (rpc->role == RPC_SERVER) {
        fd = rpc->afd;
    } else if (rpc->role == RPC_CLIENT) {
        fd = c->fd;
    }
    int ret = sock_recv(fd, buf, len);
    if (ret == -1) {
        printf("recv failed: %d\n", errno);
    }
    return ret;
}

static void on_connect(int fd, void *arg)
{
    struct rpc *rpc = (struct rpc *)arg;
    uint32_t ip;
    uint16_t port;
    rpc->afd = sock_accept(fd, &ip, &port);
    if (rpc->afd == -1) {
        printf("sock_accept failed: %d\n", errno);
        return;
    }
    rpc->on_server_init(rpc, rpc->afd, ip, port);
}

static void *event_thread(struct thread *t, void *arg)
{
    struct rpc *rpc = (struct rpc *)arg;
    gevent_base_loop(rpc->evbase);
    return NULL;
}

static void *sk_init_server(struct rpc *rpc, const char *host, uint16_t port)
{
    struct gevent *e = NULL;
    struct sk_ctx *c = calloc(1, sizeof(struct sk_ctx));
    if (!c) {
        printf("malloc failed!\n");
        goto failed;
    }
    rpc->role = RPC_SERVER;
    c->fd = sock_tcp_bind_listen(NULL, port);
    if (c->fd == -1) {
        printf("sock_tcp_bind_listen port:%d failed!\n", port);
        goto failed;
    }
    rpc->fd = c->fd;
    rpc->ctx = c;
    rpc->evbase = gevent_base_create();
    if (!rpc->evbase) {
        printf("gevent_base_create failed!\n");
        goto failed;
    }
    e = gevent_create(rpc->fd, on_connect, NULL, on_error, rpc);
    if (-1 == gevent_add(rpc->evbase, e)) {
        printf("event_add failed!\n");
    }
    rpc->dispatch_thread = thread_create(event_thread, rpc);
    return c;

failed:
    if (-1 != c->fd) {
        close(c->fd);
    }
    free(c);
    return NULL;
}

static void *sk_init_client(struct rpc *rpc, const char *host, uint16_t port)
{
    struct gevent *e = NULL;
    struct sk_ctx *c = calloc(1, sizeof(struct sk_ctx));
    if (!c) {
        printf("malloc failed!\n");
        goto failed;
    }
    rpc->role = RPC_CLIENT;

    c->connect = sock_tcp_connect(host, port);
    if (!c->connect) {
        printf("connect failed!\n");
        goto failed;
    }
    c->fd = c->connect->fd;
    if (-1 == sock_set_block(c->fd)) {
        printf("sock_set_block failed!\n");
    }
    rpc->fd = c->fd;

    rpc->ctx = c;
    rpc->evbase = gevent_base_create();
    if (!rpc->evbase) {
        printf("gevent_base_create failed!\n");
        goto failed;
    }
    e = gevent_create(rpc->fd, rpc->on_client_init, NULL, on_error, rpc);
    if (-1 == gevent_add(rpc->evbase, e)) {
        printf("event_add failed!\n");
        goto failed;
    }
    rpc->dispatch_thread = thread_create(event_thread, rpc);
    if (!rpc->dispatch_thread) {
        printf("thread_create dispatch_thread failed!\n");
        goto failed;
    }
    if (!rpc->dispatch_thread) {
        printf("thread_create dispatch_thread failed!\n");
        goto failed;
    }
    thread_lock(rpc->dispatch_thread);
    if (thread_wait(rpc->dispatch_thread, 2000) == -1) {
        printf("wait response failed %d:%s\n", errno, strerror(errno));
    }
    thread_unlock(rpc->dispatch_thread);

    return c;

failed:
    if (-1 != c->fd) {
        close(c->fd);
    }
    free(c);
    return NULL;
}

static void sk_deinit(struct rpc *rpc)
{
    struct sk_ctx *c = (struct sk_ctx *)rpc->ctx;
    close(c->fd);
    free(c);
}

static int sk_set_recv_cb(struct rpc *rpc, rpc_recv_cb *cb)
{
    sk_recv_cb = cb;
    return 0;
}

static int sk_send(struct rpc *rpc, const void *buf, size_t len)
{
    int fd = 0;
    struct sk_ctx *c = (struct sk_ctx *)rpc->ctx;
    if (rpc->role == RPC_SERVER) {
        fd = rpc->afd;
    } else if (rpc->role == RPC_CLIENT) {
        fd = c->fd;
    }
    int ret = sock_send(fd, buf, len);
    if (ret == -1) {
        printf("send failed: %d\n", errno);
    }
    return ret;
}

struct rpc_ops socket_ops = {
    .init_client      = sk_init_client,
    .init_server      = sk_init_server,
    .deinit           = sk_deinit,
    .register_recv_cb = sk_set_recv_cb,
    .send             = sk_send,
    .recv             = sk_recv,
};
