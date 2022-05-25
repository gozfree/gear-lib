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
#include "libipc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#define IPC_SERVER_NAME "/IPC_SERVER"
#define IPC_CLIENT_NAME "/IPC_CLIENT"

struct sk_ctx {
    int fd;
    struct sockaddr_un sockaddr;
};

static ipc_recv_cb *sk_recv_cb = NULL;

static void on_error(int fd, void *arg)
{
    printf("error: %d\n", errno);
}

static int sk_accept(struct ipc *ipc)
{
    struct sk_ctx *c = (struct sk_ctx *)ipc->ctx;
    socklen_t len = 0;//must not <0
    int ret = accept(c->fd, (struct sockaddr *)&c->sockaddr, &len);
    if (ret == -1) {
        printf("accept failed: %d\n", errno);
    }
    return ret;
}

static int sk_recv(struct ipc *ipc, void *buf, size_t len)
{
    int fd = 0;
    struct sk_ctx *c = (struct sk_ctx *)ipc->ctx;
    if (ipc->role == IPC_SERVER) {
        fd = ipc->afd;
    } else if (ipc->role == IPC_CLIENT) {
        fd = c->fd;
    }
    int ret = recv(fd, buf, len, 0);
    if (ret == -1) {
        printf("recv failed: %d\n", errno);
    }
    return ret;
}

static void on_recv(int fd, void *arg)
{
    struct ipc *ipc = (struct ipc *)arg;
    char buf[1024];
    int len = sk_recv(ipc, buf, sizeof(buf));
    if (sk_recv_cb) {
        sk_recv_cb(ipc, buf, len);
    } else {
        printf("sk_recv_cb is NULL!\n");
    }
}

static void on_connect(int fd, void *arg)
{
    struct ipc *ipc = (struct ipc *)arg;
    ipc->afd = sk_accept(ipc);
    if (ipc->afd == -1) {
        printf("ipc accept faileds\n");
        return;
    }
    struct gevent *e = gevent_create(ipc->afd, on_recv, NULL, on_error, (void *)ipc);
    if (-1 == gevent_add(ipc->evbase, &e)) {
        printf("event_add failed!\n");
    }
}

static void *event_thread(void *arg)
{
    struct ipc *ipc = (struct ipc *)arg;
    gevent_base_loop(ipc->evbase);
    return NULL;
}

static void *sk_init(struct ipc *ipc, uint16_t port, enum ipc_role role)
{
    char stub_name[64];
    struct sk_ctx *c = calloc(1, sizeof(struct sk_ctx));
    if (!c) {
        printf("malloc failed!\n");
        goto failed;
    }
    if (-1 == (c->fd = socket(PF_UNIX, SOCK_SEQPACKET, 0))) {
        printf("socket failed: %d\n", errno);
        goto failed;
    }
    ipc->fd = c->fd;
    struct gevent *e = NULL;
    snprintf(stub_name, sizeof(stub_name), "%s.%d", IPC_SERVER_NAME, port);
    c->sockaddr.sun_family = PF_UNIX;
    snprintf(c->sockaddr.sun_path, sizeof(c->sockaddr.sun_path), "/tmp/%s", stub_name);
    if (role == IPC_SERVER) {
        if (-1 == bind(c->fd, (struct sockaddr*)&c->sockaddr, sizeof(struct sockaddr_un))) {
            printf("bind %s failed: %d\n", stub_name, errno);
            goto failed;
        }
        if (-1 == listen(c->fd, SOMAXCONN)) {
            printf("listen failed: %d\n", errno);
            goto failed;
        }
        e = gevent_create(ipc->fd, on_connect, NULL, on_error, ipc);
    } else if (role == IPC_CLIENT) {
        if (connect(c->fd, (struct sockaddr*)&c->sockaddr, sizeof(struct sockaddr_un)) < 0) {
            printf("connect %s failed: %d\n", stub_name, errno);
            goto failed;
        }
        e = gevent_create(ipc->fd, on_recv, NULL, on_error, ipc);
    }
    ipc->evbase = gevent_base_create();
    if (!ipc->evbase) {
        printf("gevent_base_create failed!\n");
        goto failed;
    }
    if (-1 == gevent_add(ipc->evbase, &e)) {
        printf("event_add failed!\n");
    }
    pthread_create(&ipc->tid, NULL, event_thread, ipc);
    return c;

failed:
    if (-1 != c->fd) {
        close(c->fd);
    }
    free(c);
    return NULL;
}

static void sk_deinit(struct ipc *ipc)
{
    struct sk_ctx *c = (struct sk_ctx *)ipc->ctx;
    close(c->fd);
    if (!access(c->sockaddr.sun_path, F_OK)) {
        remove(c->sockaddr.sun_path);
    }
    free(c);
}

static int sk_set_recv_cb(struct ipc *ipc, ipc_recv_cb *cb)
{
    sk_recv_cb = cb;
    return 0;
}

static int sk_send(struct ipc *ipc, const void *buf, size_t len)
{
    int fd = 0;
    struct sk_ctx *c = (struct sk_ctx *)ipc->ctx;
    if (ipc->role == IPC_SERVER) {
        fd = ipc->afd;
    } else if (ipc->role == IPC_CLIENT) {
        fd = c->fd;
    }
    int ret = send(fd, buf, len, 0);
    if (ret == -1) {
        printf("send failed: %d\n", errno);
    }
    return ret;
}

struct ipc_ops socket_ops = {
    .init             = sk_init,
    .deinit           = sk_deinit,
    .accept           = NULL,
    .connect          = NULL,
    .register_recv_cb = sk_set_recv_cb,
    .send             = sk_send,
    .recv             = sk_recv,
};
