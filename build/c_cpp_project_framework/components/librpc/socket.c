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
#include <libposix.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#define MAX_UUID_LEN                (21)

struct socket_ctx {
    /* fd:
     * server: only for bind and listen, and triggerd when new connect coming
     * client: only for connection, and trigger when server response
     */
    int fd;
    struct hash *hash_fd2conn;
    struct sock_connection *connect;
};

static struct sock_connection *find_connection(struct socket_ctx *c, int fd)
{
    return hash_get32(c->hash_fd2conn, fd);
}

static void on_error(int fd, void *arg)
{
    printf("error: %d\n", errno);
}

static uint32_t create_uuid(int fd, uint32_t ip, uint16_t port)
{
    char uuid[MAX_UUID_LEN];
    snprintf(uuid, MAX_UUID_LEN, "%08x%08x%04x", fd, ip, port);
    return hash_gen32(uuid, sizeof(uuid));
}

static void on_recv(int fd, void *arg)
{
    struct rpcs *s = (struct rpcs *)arg;
    struct rpc_session *session = hash_get32(s->hash_fd2session, fd);
    session->base.fd = fd;
    s->on_message(s, session);
}

static void on_xxx(int fd, void *arg)
{
    //printf("on_xxx fd=%d\n", fd);
}

static void on_connect_of_server(int fd, void *arg)
{
    struct rpc_base *r = (struct rpc_base *)arg;
    struct socket_ctx *c = (struct socket_ctx *)r->ctx;
    struct rpcs *s = container_of(r, struct rpcs, base);
    struct rpc_session *session;
    struct sock_connection *conn;
    struct gevent *e;
    uint32_t uuid;
    char ip_str[SOCK_ADDR_LEN];
    c->connect = sock_accept_connect(fd);
    if (!c->connect) {
        printf("sock_accept failed: %d\n", errno);
        return;
    }

    conn = hash_get32(c->hash_fd2conn, c->connect->fd);
    if (conn) {
        printf("connection of fd=%d seems already exist!\n", c->connect->fd);
        return;
    }
    hash_set32(c->hash_fd2conn, c->connect->fd, c->connect);

    e = gevent_create(c->connect->fd, on_recv, on_xxx, on_error, s);
    if (-1 == gevent_add(s->base.evbase, &e)) {
        printf("event_add failed!\n");
    }
    da_push_back(r->ev_list, &e);
    sock_addr_ntop(ip_str, c->connect->remote.ip);
    uuid = create_uuid(fd, c->connect->remote.ip, c->connect->remote.port);
    session = s->on_create_session(s, c->connect->fd, uuid);
    if (!session) {
        printf("create rpc session failed!\n");
    }

    hash_set32(s->hash_fd2session, c->connect->fd, session);
    printf("new connect: %s:%d fd=%d, uuid:0x%08x\n", ip_str, c->connect->remote.port, c->connect->fd, uuid);
}

static int socket_init_server(struct rpc_base *r, const char *host, uint16_t port)
{
    struct gevent *e = NULL;
    struct socket_ctx *c = calloc(1, sizeof(struct socket_ctx));
    if (!c) {
        printf("malloc failed!\n");
        goto failed;
    }
    c->hash_fd2conn = hash_create(1024);
    c->fd = sock_tcp_bind_listen(NULL, port);
    if (c->fd == -1) {
        printf("sock_tcp_bind_listen port:%d failed!\n", port);
        goto failed;
    }
    r->fd = c->fd;
    r->ctx = c;
    e = gevent_create(r->fd, on_connect_of_server, on_xxx, on_error, r);
    if (-1 == gevent_add(r->evbase, &e)) {
        printf("event_add failed!\n");
        goto failed;
    }
    da_push_back(r->ev_list, &e);
    return 0;

failed:
    if (-1 != c->fd) {
        close(c->fd);
    }
    free(c);
    return -1;
}

static void on_connect_of_client(int fd, void *arg)
{
    struct rpc_base *r = (struct rpc_base *)arg;
    struct rpc_session *ss = container_of(r, struct rpc_session, base);
    struct rpc *rpc = container_of(ss, struct rpc, session);
    rpc->on_connect_server(rpc);
}

static int socket_init_client(struct rpc_base *r, const char *host, uint16_t port)
{
    struct gevent *e = NULL;
    struct socket_ctx *c = calloc(1, sizeof(struct socket_ctx));
    if (!c) {
        printf("malloc failed!\n");
        goto failed;
    }
    c->hash_fd2conn = hash_create(1024);
    c->connect = sock_tcp_connect(host, port);
    if (!c->connect) {
        printf("connect %s:%d failed!\n", host, port);
        goto failed;
    }
    c->fd = c->connect->fd;
    hash_set32(c->hash_fd2conn, c->connect->fd, c->connect);
    if (-1 == sock_set_block(c->fd)) {
        printf("sock_set_block failed!\n");
    }
    r->fd = c->fd;
    r->ctx = c;
    e = gevent_create(r->fd, on_connect_of_client, on_xxx, on_error, r);
    if (-1 == gevent_add(r->evbase, &e)) {
        printf("event_add failed!\n");
        goto failed;
    }
    thread_lock(r->dispatch_thread);
    if (thread_wait(r->dispatch_thread, 2000) == -1) {
        printf("%s wait response failed %d:%s\n", __func__, errno, strerror(errno));
    }
    thread_unlock(r->dispatch_thread);

    return 0;

failed:
    if (-1 != c->fd) {
        close(c->fd);
    }
    free(c);
    return -1;
}

static void socket_deinit(struct rpc_base *r)
{
    struct socket_ctx *c = (struct socket_ctx *)r->ctx;
    close(c->fd);
    free(c);
}

static int socket_send(struct rpc_base *r, const void *buf, size_t len)
{
    int ret;
    struct socket_ctx *c = (struct socket_ctx *)r->ctx;
    struct sock_connection *conn = find_connection(c, r->fd);
    if (!conn) {
        printf("find connection fd=%d failed!\n", r->fd);
        return -1;
    }
    ret = sock_send(conn->fd, buf, len);
    if (ret == -1) {
        printf("send failed: %d\n", errno);
    }
    return ret;
}

static int socket_recv(struct rpc_base *r, void *buf, size_t len)
{
    int ret;
    struct socket_ctx *c = (struct socket_ctx *)r->ctx;
    struct sock_connection *conn = find_connection(c, r->fd);
    if (!conn) {
        printf("find connection fd=%d failed!\n", r->fd);
        return -1;
    }
    ret = sock_recv(conn->fd, buf, len);
    if (ret == -1) {
        printf("recv failed fd=%d, rpc_base=%p: %d\n", c->fd, r, errno);
    }
    return ret;
}

struct rpc_ops socket_ops = {
    .init_client      = socket_init_client,
    .init_server      = socket_init_server,
    .deinit           = socket_deinit,
    .send             = socket_send,
    .recv             = socket_recv,
};
