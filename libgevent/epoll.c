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
#include "libgevent.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <sys/epoll.h>

#ifdef __ANDROID__
#define EPOLLRDHUP      (0x2000)
#endif

#define EPOLL_MAX_NEVENT    (4096)
#define MAX_SECONDS_IN_MSEC_LONG \
        (((LONG_MAX) - 999) / 1000)

struct epoll_ctx {
    int epfd;
    int nevents;
    struct epoll_event *events;
};

static void *epoll_init(void)
{
    int fd;
    struct epoll_ctx *ec;
    fd = epoll_create(1);
    if (-1 == fd) {
        printf("errno=%d %s\n", errno, strerror(errno));
        return NULL;
    }
    ec = (struct epoll_ctx *)calloc(1, sizeof(struct epoll_ctx));
    if (!ec) {
        printf("malloc epoll_ctx failed!\n");
        return NULL;
    }
    ec->epfd = fd;
    ec->nevents = EPOLL_MAX_NEVENT;
    ec->events = (struct epoll_event *)calloc(EPOLL_MAX_NEVENT, sizeof(struct epoll_event));
    if (!ec->events) {
        printf("malloc epoll_event failed!\n");
        return NULL;
    }
    return ec;
}

static void epoll_deinit(void *ctx)
{
    struct epoll_ctx *ec = (struct epoll_ctx *)ctx;
    if (!ctx) {
        return;
    }
    free(ec->events);
    free(ec);
}

static int epoll_add(struct gevent_base *eb, struct gevent *e)
{
    struct epoll_ctx *ec = (struct epoll_ctx *)eb->ctx;
    struct epoll_event epev;
    memset(&epev, 0, sizeof(epev));
    if (e->flags & EVENT_READ)
        epev.events |= EPOLLIN;
    if (e->flags & EVENT_WRITE)
        epev.events |= EPOLLOUT;
    if (e->flags & EVENT_ERROR)
        epev.events |= EPOLLERR;
    epev.events |= EPOLLET;
    epev.data.ptr = (void *)e;

    if (-1 == epoll_ctl(ec->epfd, EPOLL_CTL_ADD, e->evfd, &epev)) {
        printf("errno=%d %s\n", errno, strerror(errno));
        return -1;
    }
    return 0;
}

static int epoll_del(struct gevent_base *eb, struct gevent *e)
{
    struct epoll_ctx *ec = (struct epoll_ctx *)eb->ctx;
    if (-1 == epoll_ctl(ec->epfd, EPOLL_CTL_DEL, e->evfd, NULL)) {
        perror("epoll_ctl");
        return -1;
    }
    return 0;
}

static int epoll_dispatch(struct gevent_base *eb, struct timeval *tv)
{
    struct epoll_ctx *epop = (struct epoll_ctx *)eb->ctx;
    struct epoll_event *events = epop->events;
    int i, n;
    int timeout = -1;

    if (tv != NULL) {
        if (tv->tv_usec > 1000000 || tv->tv_sec > MAX_SECONDS_IN_MSEC_LONG)
            timeout = -1;
        else
            timeout = (tv->tv_sec * 1000) + ((tv->tv_usec + 999) / 1000);
    } else {
        timeout = -1;
    }
    n = epoll_wait(epop->epfd, events, epop->nevents, timeout);
    if (-1 == n) {
        if (errno != EINTR) {
            printf("epoll_wait failed %d: %s\n", errno, strerror(errno));
            return -1;
        }
        return 0;
    }
    if (0 == n) {
        printf("epoll_wait timeout\n");
        return 0;
    }
    for (i = 0; i < n; i++) {
        int what = events[i].events;
        struct gevent *e = (struct gevent *)events[i].data.ptr;

        if (what & (EPOLLHUP|EPOLLERR)) {
        } else {
            if (what & EPOLLIN)
                e->evcb->ev_in(e->evfd, (void *)e->evcb->args);
            if (what & EPOLLOUT)
                e->evcb->ev_out(e->evfd, (void *)e->evcb->args);
            if (what & EPOLLRDHUP)
                e->evcb->ev_err(e->evfd, (void *)e->evcb->args);
        }
    }
    return 0;
}

struct gevent_ops epollops = {
    .init     = epoll_init,
    .deinit   = epoll_deinit,
    .add      = epoll_add,
    .del      = epoll_del,
    .dispatch = epoll_dispatch,
};
