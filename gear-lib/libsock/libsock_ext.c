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
#include "libsock_ext.h"
#include <libgevent.h>
#include "libsock.h"
#ifdef ENABLE_PTCP
#include "libptcp.h"
#endif
#include <errno.h>

static void on_error(int fd, void *arg)
{
    printf("error: %d\n", errno);
}

static void on_recv(int fd, void *arg)
{
    struct sock_server *s;
    char buf[2048];
    int ret=0;
    memset(buf, 0, sizeof(buf));
    s = (struct sock_server *)arg;
    ret = sock_recv(fd, buf, 2048);
    if (ret > 0) {
        s->on_buffer(s, buf, ret);
    } else if (ret == 0) {
        printf("delete connection fd:%d\n", fd);
        if (s->on_disconnect) {
            s->on_disconnect(s, NULL);
        }
    } else if (ret < 0) {
        printf("%s:%d recv failed!\n", __func__, __LINE__);
    }
}

static void on_client_recv(int fd, void *arg)
{
    struct sock_client *c;
    char buf[2048];
    int ret=0;
    memset(buf, 0, sizeof(buf));
    c = (struct sock_client *)arg;
    ret = sock_recv(fd, buf, 2048);
    if (ret > 0) {
        c->on_buffer(c, buf, ret);
    } else if (ret == 0) {
        printf("delete connection fd:%d\n", fd);
        if (c->on_disconnect) {
            c->on_disconnect(c, NULL);
        }
    } else if (ret < 0) {
        printf("%s:%d recv failed!\n", __func__, __LINE__);
    }
}

#ifdef ENABLE_PTCP
static void on_ptcp_recv(int fd, void *arg)
{
    char buf[2048];
    int ret=0;
    memset(buf, 0, sizeof(buf));
    struct sock_server *s = (struct sock_server *)arg;
    ret = sock_recv(s->fd64, buf, 2048);
    if (ret > 0) {
        s->on_buffer(fd, buf, ret);
    } else if (ret == 0) {
        printf("delete connection fd:%d\n", fd);
        if (s->on_disconnect) {
            s->on_disconnect(fd, NULL);
        }
    } else if (ret < 0) {
        printf("%s:%d recv failed!\n", __func__, __LINE__);
    }
}
#endif

static void on_tcp_connect(int fd, void *arg)
{
    int afd;
    uint32_t ip;
    uint16_t port;
    struct gevent *e = NULL;
    struct sock_server *s = (struct sock_server *)arg;
    struct sock_connection sc;

    afd = sock_accept(fd, &ip, &port);
    if (afd == -1) {
        printf("errno=%d %s\n", errno, strerror(errno));
        return;
    }
    if (s->on_connect) {
        sc.fd = afd;
        sc.type = SOCK_STREAM;
        if (-1 == sock_getaddr_by_fd(sc.fd, &sc.local)) {
            printf("sock_getaddr_by_fd failed: %s\n", strerror(errno));
        }
        sc.remote.ip = ip;
        sc.remote.port = port;
        sock_addr_ntop(sc.remote.ip_str, ip);
        s->on_connect(s, &sc);
    }
    e = gevent_create(afd, on_recv, NULL, on_error, s);
    if (-1 == gevent_add(s->evbase, &e)) {
        printf("event_add failed!\n");
    }
}

#ifdef ENABLE_PTCP
static void on_ptcp_connect(int fd, void *arg)
{
    int afd;
    uint32_t ip;
    uint16_t port;
    struct gevent *e = NULL;
    struct sock_server *s = (struct sock_server *)arg;
    struct sock_connection sc;

    afd = sock_accept(s->fd64, &ip, &port);
    if (afd == -1) {
        printf("errno=%d %s\n", errno, strerror(errno));
        return;
    }
    if (s->on_connect) {
        sc.fd = afd;
        sc.type = SOCK_STREAM;
        if (-1 == sock_getaddr_by_fd(sc.fd, &sc.local)) {
            printf("sock_getaddr_by_fd failed: %s\n", strerror(errno));
        }
        sc.remote.ip = ip;
        sc.remote.port = port;
        sock_addr_ntop(sc.remote.ip_str, ip);
        s->on_connect(fd, &sc);
    }
    e = gevent_create(afd, on_ptcp_recv, NULL, on_error, s);
    if (-1 == gevent_mod(s->evbase, &e)) {
        printf("event_add failed!\n");
    }
}
#endif

struct sock_server *sock_server_create(const char *host, uint16_t port, enum sock_type type)
{
    struct sock_server *s;
    if (type > SOCK_TYPE_MAX) {
        printf("invalid paraments\n");
        return NULL;
    }
    s = calloc(1, sizeof(struct sock_server));
    if (!s) {
        printf("malloc sock_server failed!\n");
        return NULL;
    }
    s->type = type;
    switch (type) {
    case SOCK_TYPE_TCP:
        s->fd = sock_tcp_bind_listen(host, port);
        break;
    case SOCK_TYPE_UDP:
        s->fd = sock_udp_bind(host, port);
        //sock_set_noblk(s->fd, true);
        break;
#ifdef ENABLE_PTCP
    case SOCK_TYPE_PTCP:
        s->fd64 = sock_ptcp_bind_listen(host, port);
        break;
#endif
    default:
        printf("invalid sock_type!\n");
        break;
    }

    s->evbase = gevent_base_create();
    if (!s->evbase) {
        printf("gevent_base_create failed!\n");
        return NULL;
    }
    return s;
}

int sock_server_set_callback(struct sock_server *s,
        void (*on_connect)(struct sock_server *s, struct sock_connection *conn),
        void (*on_buffer)(struct sock_server *s, void *buf, size_t len),
        void (*on_disconnect)(struct sock_server *s, struct sock_connection *conn))
{
    struct gevent *e;
    if (!s) {
        return -1;
    }
    s->on_connect = on_connect;
    s->on_buffer = on_buffer;
    s->on_disconnect = on_disconnect;
    switch (s->type) {
    case SOCK_TYPE_UDP:
        e = gevent_create(s->fd, on_recv, NULL, on_error, s);
        break;
    case SOCK_TYPE_TCP:
        e = gevent_create(s->fd, on_tcp_connect, NULL, on_error, s);
        break;
#ifdef ENABLE_PTCP
    case SOCK_TYPE_PTCP: {
        ptcp_socket_t ptcp = *(ptcp_socket_t *) &s->fd64;
        int fd = ptcp_get_socket_fd(ptcp);
        printf("ptcp_get_socket_fd fd=%d\n", fd);
        e = gevent_create(fd, on_ptcp_connect, NULL, on_error, s);
    } break;
#endif
    default:
        break;
    }
    if (-1 == gevent_add(s->evbase, &e)) {
        printf("event_add failed!\n");
        return -1;
    }
    return 0;
}

int sock_server_dispatch(struct sock_server *s)
{
    if (!s) {
        return -1;
    }
    gevent_base_loop(s->evbase);
    return 0;
}

void sock_server_destroy(struct sock_server *s)
{
    if (!s) {
        return;
    }
    gevent_base_loop_break(s->evbase);
    gevent_base_destroy(s->evbase);
}


struct sock_client *sock_client_create(const char *host, uint16_t port, enum sock_type type)
{
    struct sock_client *c;
    if (type > SOCK_TYPE_MAX) {
        printf("invalid paraments\n");
        return NULL;
    }
    c = calloc(1, sizeof(struct sock_client));
    if (!c) {
        printf("malloc sock_client failed!\n");
        return NULL;
    }
    c->host = strdup(host);
    c->port = port;
    c->type = type;
    c->evbase = gevent_base_create();
    if (!c->evbase) {
        printf("gevent_base_create failed!\n");
        return NULL;
    }
    return c;
}

int sock_client_set_callback(struct sock_client *c,
        void (*on_connect)(struct sock_client *c, struct sock_connection *conn),
        void (*on_buffer)(struct sock_client *c, void *buf, size_t len),
        void (*on_disconnect)(struct sock_client *c, struct sock_connection *conn))
{
    if (!c) {
        return -1;
    }
    c->on_connect = on_connect;
    c->on_buffer = on_buffer;
    c->on_disconnect = on_disconnect;

    return 0;
}

static void *sock_client_thread(struct thread *thread, void *arg)
{
    struct sock_client *c = (struct sock_client *)arg;

    gevent_base_loop(c->evbase);
    return NULL;
}

GEAR_API int sock_client_connect(struct sock_client *c)
{
    struct gevent *e;
    c->conn = sock_tcp_connect(c->host, c->port);
    if (!c->conn) {
        printf("sock_tcp_connect %s:%d failed!\n", c->host, c->port);
        return -1;
    }
    switch (c->type) {
    case SOCK_TYPE_TCP:
    case SOCK_TYPE_UDP:
        c->fd = c->conn->fd;
        break;
#ifdef ENABLE_PTCP
    case SOCK_TYPE_PTCP:
        c->fd = c->conn->fd64;
        break;
#endif
    default:
        printf("invalid sock_type!\n");
        break;
    }
    e = gevent_create(c->fd, on_client_recv, NULL, on_error, c);
    if (-1 == gevent_add(c->evbase, &e)) {
        printf("event_add failed!\n");
    }
    if (c->conn) {
        if (c->on_connect) {
            c->on_connect(c, c->conn);
        }
    }
    c->thread = thread_create(sock_client_thread, c);

    return 0;
}
