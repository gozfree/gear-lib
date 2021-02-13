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
#include <errno.h>

static void on_error(int fd, void *arg)
{
    printf("error: %d\n", errno);
}

static void on_recv(int fd, void *arg)
{
    char buf[2048];
    int ret=0;
    memset(buf, 0, sizeof(buf));
    struct sock_server *s = (struct sock_server *)arg;
    ret = sock_recv(fd, buf, 2048);
    if (ret > 0) {
        s->on_buffer(fd, buf, ret);
    } else if (ret == 0) {
        printf("delete connection fd:%d\n", fd);
    } else if (ret < 0) {
        printf("recv failed!\n");
    }
}

static void tcp_on_connect(int fd, void *arg)
{
    char str_ip[INET_ADDRSTRLEN];
    int afd;
    uint32_t ip;
    uint16_t port;
    struct gevent *e =NULL;
    struct sock_server *s = (struct sock_server *)arg;
    afd = sock_accept(fd, &ip, &port);
    if (afd == -1) {
        printf("errno=%d %s\n", errno, strerror(errno));
        return;
    }
    sock_addr_ntop(str_ip, ip);
    printf("new connect comming: ip = %s, port = %d\n", str_ip, port);
    if (0 > sock_set_nonblock(afd)) {
        printf("sock_set_nonblock failed!\n");
        return;
    }
    e = gevent_create(afd, on_recv, NULL, on_error, s);
    if (-1 == gevent_add2(s->evbase, &e)) {
        printf("event_add failed!\n");
    }
}

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
        sock_set_noblk(s->fd, true);
        break;
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
        void (on_buffer)(int, void *arg, size_t len))
{
    struct gevent *e;
    if (!s) {
        return -1;
    }
    s->on_buffer = on_buffer;
    switch (s->type) {
    case SOCK_TYPE_UDP:
        e = gevent_create(s->fd, on_recv, NULL, on_error, s);
        break;
    case SOCK_TYPE_TCP:
        e = gevent_create(s->fd, tcp_on_connect, NULL, on_error, s);
        break;
    default:
        break;
    }
    if (-1 == gevent_add2(s->evbase, &e)) {
        printf("event_add failed!\n");
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

}
