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
#include "libptcp.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include "pseudotcp.h"

#define MAX_EPOLL_EVENT 16

#define DEFAULT_TCP_MTU 1400 /* Use 1400 because of VPNs and we assume IEE 802.3 */

#ifndef container_of
#define container_of(ptr, type, member) ({			\
	const typeof( ((type *)0)->member ) *__mptr = (ptr);	\
	(type *)( (char *)__mptr - offsetof(type,member) );})
#endif


typedef struct _PseudoTcpSocket PseudoTcpSocket;
typedef struct _ptcp_socket_t {
    PseudoTcpSocket     *sock;
    int                 fd;
    struct sockaddr     sa;
    socklen_t           sa_len;
    timer_t             timer_id;
    sem_t               sem;
} ptcp_t;

static int sock_addr_ntop(char *str, size_t len, uint32_t ip)
{
    struct in_addr ia;

    ia.s_addr = ip;
    if (NULL == inet_ntop(AF_INET, &ia, str, len)) {
        printf("inet_ntop: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

static void _ptcp_on_opened(PseudoTcpSocket *sock, gpointer data)
{
    ptcp_t *ptcp = data;
    sem_post(&ptcp->sem);
}

static void _ptcp_on_readable(PseudoTcpSocket *tcp, gpointer data)
{
    //printf("%s:%d xxxx\n", __func__, __LINE__);
}

static void _ptcp_on_writable(PseudoTcpSocket *tcp, gpointer data)
{
    //printf("%s:%d xxxx\n", __func__, __LINE__);
}

static void _ptcp_on_closed(PseudoTcpSocket *tcp, guint32 error, gpointer data)
{
    //printf("%s:%d xxxx\n", __func__, __LINE__);
}

static timer_t timeout_add(uint64_t msec, void (*func)(union sigval sv), void *data)
{
    timer_t timerid = (timer_t)-1;
    struct sigevent sev;

    memset(&sev, 0, sizeof(struct sigevent));

    sev.sigev_notify = SIGEV_THREAD;
    sev.sigev_notify_function = func;
    sev.sigev_value.sival_ptr = data;

    if (timer_create(CLOCK_REALTIME, &sev, &timerid) == -1) {
        perror("timer_create");
    }

    return timerid;
}

static int timeout_del(timer_t tid)
{
    return timer_delete(tid);
}

static void clock_reset(ptcp_t *ptcp, uint64_t timeout)
{
    struct itimerspec its;

    its.it_value.tv_sec = timeout / 1000; 
    its.it_value.tv_nsec = timeout % 1000 * 1000000;
    its.it_interval.tv_sec = 0; 
    its.it_interval.tv_nsec = 0; 
    if (timer_settime(ptcp->timer_id, 0, &its, NULL) < 0) {
        perror("timer_settime");
        printf("time = %ld s, %ld ns\n", 
                    its.it_value.tv_sec, its.it_value.tv_nsec );
    }
}

static void adjust_clock(ptcp_t *ptcp)
{
    PseudoTcpSocket *sock = ptcp->sock;
    guint64 timeout = 0;

    if (pseudo_tcp_socket_get_next_clock (sock, &timeout)) {
        timeout -= g_get_monotonic_time () / 1000;
        //printf("Socket %p: Adjusting clock to %" PRIu64 " ms\n", sock, timeout);
        clock_reset(ptcp, timeout);
#if 0
        if (left_clock != 0)
                g_source_remove (left_clock);
        left_clock = g_timeout_add (timeout, notify_clock, sock);
#endif
    } else {
            g_debug ("Socket %p should be destroyed, it's done", sock);
    }
}

static PseudoTcpWriteResult _ptcp_write_packet(PseudoTcpSocket *sock,
                            const char *buffer, uint32_t len, void *user_data)
{
    ptcp_t *ptcp = user_data;
    if (pseudo_tcp_socket_is_closed(ptcp->sock)) {
        printf("Stream: pseudo TCP socket got destroyed.");
    } else {
        ssize_t res;
        res = sendto(ptcp->fd, buffer, len, 0, &ptcp->sa, sizeof(struct sockaddr));
        if (-1 == res) {
            printf("sendto error: %s\n", strerror(errno));
        }
        struct sockaddr_in *si = (struct sockaddr_in *)&ptcp->sa;
        char ip[16];
        sock_addr_ntop(ip, sizeof(ip), si->sin_addr.s_addr);
        //printf("%s:%d sendto res=%zu, ip=%s, port=%d\n", __func__, __LINE__, res, ip, ntohs(si->sin_port));
        adjust_clock(ptcp);
        if (res < len) {
            printf("sendto len=%d, res=%d\n", (int)len, (int)res);
            return WR_FAIL;
        }
        return WR_SUCCESS;
    }
    return WR_FAIL;
}

static PseudoTcpWriteResult ptcp_read(ptcp_t *ptcp, void *buf, size_t len)
{
    ssize_t res;
#define RECV_BUF_LEN (64*1024)
    char rbuf[RECV_BUF_LEN] = {0};
    ptcp->sa_len = sizeof(struct sockaddr);
    res = recvfrom(ptcp->fd, rbuf, sizeof(rbuf), MSG_DONTWAIT, &ptcp->sa, &ptcp->sa_len);
    if (-1 == res) {
        printf("recvfrom error: %s\n", strerror(errno));
        return WR_FAIL;
    }
    struct sockaddr_in *si = (struct sockaddr_in *)&ptcp->sa;
    char ip[16];
    sock_addr_ntop(ip, sizeof(ip), si->sin_addr.s_addr);
    //printf("%s:%d recvfrom res=%zu, ip=%s, port=%d\n", __func__, __LINE__, res, ip, ntohs(si->sin_port));
    if (TRUE == pseudo_tcp_socket_notify_packet(ptcp->sock, rbuf, res)) {
        adjust_clock(ptcp);
        return WR_SUCCESS;
    }
    return WR_FAIL;
}

static void notify_clock(union sigval sv)
{
    void *data = (void *)sv.sival_ptr;
    ptcp_t *ptcp = (ptcp_t *)data;
    pseudo_tcp_socket_notify_clock(ptcp->sock);
    adjust_clock(ptcp);
}

ptcp_socket_t ptcp_socket_by_fd(int fd)
{
    if (fd <= 0) {
        printf("%s fd invalid!\n", __func__);
        return NULL;
    }
    ptcp_t *ptcp = calloc(1, sizeof(ptcp_t));
    if (!ptcp) {
        printf("malloc ptcp failed!\n");
        return NULL;
    }
    PseudoTcpCallbacks pseudo_tcp_callbacks = {
        ptcp,
        _ptcp_on_opened,
        _ptcp_on_readable,
        _ptcp_on_writable,
        _ptcp_on_closed,
        _ptcp_write_packet
    };
    pseudo_tcp_set_debug_level(PSEUDO_TCP_DEBUG_NONE);
    PseudoTcpSocket *sock = pseudo_tcp_socket_new(1, &pseudo_tcp_callbacks);
    if (!sock) {
        printf("pseudo_tcp_socket_new failed!\n");
        return NULL;
    }
    pseudo_tcp_socket_notify_mtu(sock, DEFAULT_TCP_MTU);
    ptcp->sock = sock;
    ptcp->fd = fd;
    sem_init(&ptcp->sem, 0, 0);
    ptcp->timer_id = timeout_add(0, notify_clock, ptcp);
    printf("%s:%d ptcp_socket success ptcp=%p, fd = %d\n", __func__, __LINE__, ptcp, ptcp->fd);
    return &ptcp->fd;
}

ptcp_socket_t ptcp_socket()
{
    int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (-1 == fd) {
        return NULL;
    }
    return ptcp_socket_by_fd(fd);
}

int ptcp_get_socket_fd(ptcp_socket_t sock)
{
    ptcp_t *ptcp = container_of(sock, ptcp_t, fd);
    return ptcp->fd;
}

int ptcp_bind(ptcp_socket_t sock, const struct sockaddr *sa, socklen_t addrlen)
{
    ptcp_t *ptcp = container_of(sock, ptcp_t, fd);
    if (-1 == bind(ptcp->fd, sa, addrlen)) {
        struct sockaddr_in *si = (struct sockaddr_in *)sa;
        char ip[16];
        sock_addr_ntop(ip, sizeof(ip), si->sin_addr.s_addr);
        printf("bind fd = %d, ip = %s: port = %d failed: %d\n", ptcp->fd, ip, ntohs(si->sin_port), errno);
        return -1;
    }
    memcpy(&ptcp->sa, sa, addrlen);
    return 0;
}

static void ptcp_event_read(void *arg)
{
    char buf[10240] = {0};
    ptcp_t *ptcp = (ptcp_t *)arg;
    if (WR_SUCCESS == ptcp_read(ptcp, buf, sizeof(buf))) {
    }
}

static void *ptcp_event_loop(void *arg)
{
    ptcp_t *ptcp = (ptcp_t *)arg;
    int epfd;
    int ret, i;
    struct epoll_event evbuf[MAX_EPOLL_EVENT];
    struct epoll_event event;

    int fd = ptcp->fd;
    epfd = epoll_create(1);
    if (epfd == -1) {
        perror("epoll_create");
        return NULL;
    }

    memset(&event, 0, sizeof(event));
    event.data.ptr = ptcp;
    event.events = EPOLLIN | EPOLLET;

    if (-1 == epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &event)) {
        perror("epoll_ctl");
        close(epfd);
        return NULL;
    }

    while (1) {
        ret = epoll_wait(epfd, evbuf, MAX_EPOLL_EVENT, -1);
        if (ret == -1) {
            perror("epoll_wait");
            continue;
        }
        for (i = 0; i < ret; i++) {
            if (evbuf[i].data.fd == -1)
                continue;
            if (evbuf[i].events & (EPOLLERR | EPOLLHUP)) {
                perror("epoll error");
            }
            if (evbuf[i].events & EPOLLOUT) {
                perror("epoll out");
            }
            if (evbuf[i].events & EPOLLIN) {
                ptcp_event_read(evbuf[i].data.ptr);
            }
        }
    }
    return NULL;
}

int ptcp_listen(ptcp_socket_t sock, int backlog)
{
    ptcp_t *ptcp = container_of(sock, ptcp_t, fd);
    pthread_t tid;
    pthread_create(&tid, NULL, ptcp_event_loop, ptcp);
    return 0;
}

int ptcp_accept(ptcp_socket_t sock, struct sockaddr *addr, socklen_t *addrlen)
{
    ptcp_t *ptcp = container_of(sock, ptcp_t, fd);
    sem_wait(&ptcp->sem);
    *addrlen = ptcp->sa_len;
    memcpy(addr, &ptcp->sa, *addrlen);
    struct sockaddr_in *si = (struct sockaddr_in *)&ptcp->sa;
    char ip[16];
    sock_addr_ntop(ip, sizeof(ip), si->sin_addr.s_addr);
    return ptcp->fd;
}

int ptcp_close_by_fd(ptcp_socket_t sock, int fd)
{
    ptcp_t *ptcp = container_of(sock, ptcp_t, fd);
    if (!ptcp) {
        printf("invalid paraments\n");
        return -1;
    }
    pseudo_tcp_socket_close(ptcp->sock, true);
    pseudo_tcp_socket_delete(ptcp->sock);
    sem_destroy(&ptcp->sem);
    timeout_del(ptcp->timer_id);
    free(ptcp);
    return 0;
}

int ptcp_close(ptcp_socket_t sock)
{
    ptcp_t *ptcp = container_of(sock, ptcp_t, fd);
    int fd = ptcp->fd;
    ptcp_close_by_fd(sock, fd);
    return close(fd);
}

int ptcp_connect(ptcp_socket_t sock, const struct sockaddr *sa, socklen_t addrlen)
{
    ptcp_t *ptcp = container_of(sock, ptcp_t, fd);
    pthread_t tid;
    if (-1 == connect(ptcp->fd, sa, addrlen)) {
        printf("connect fd=%d error: %s\n", ptcp->fd, strerror(errno));
        return -1;
    }
    memcpy(&ptcp->sa, sa, addrlen);

    pthread_create(&tid, NULL, ptcp_event_loop, ptcp);
    if (FALSE == pseudo_tcp_socket_connect(ptcp->sock)) {
        printf("pseudo_tcp_socket_connect failed!\n");
    }
    sem_wait(&ptcp->sem);
    return 0;
}

ssize_t ptcp_recv(ptcp_socket_t sock, void *buf, size_t len, int flags)
{
    ptcp_t *ptcp = container_of(sock, ptcp_t, fd);
    return pseudo_tcp_socket_recv(ptcp->sock, buf, len);
}

ssize_t ptcp_send(ptcp_socket_t sock, const void *buf, size_t len, int flags)
{
    ptcp_t *ptcp = container_of(sock, ptcp_t, fd);
    return pseudo_tcp_socket_send(ptcp->sock, buf, len);
}
