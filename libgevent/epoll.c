/*****************************************************************************
 * Copyright (C) 2014-2015
 * file:    epoll.c
 * author:  gozfree <gozfree@163.com>
 * created: 2015-04-27 00:59
 * updated: 2015-07-12 00:41
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <sys/epoll.h>
#include <libgzf.h>
#include <liblog.h>
#include "libgevent.h"

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

static void *epoll_init()
{
    int fd;
    struct epoll_ctx *ec;
    fd = epoll_create(1);
    if (-1 == fd) {
        loge("errno=%d %s\n", errno, strerror(errno));
        return NULL;
    }
    ec = (struct epoll_ctx *)calloc(1, sizeof(struct epoll_ctx));
    if (!ec) {
        loge("malloc epoll_ctx failed!\n");
        return NULL;
    }
    ec->epfd = fd;
    ec->nevents = EPOLL_MAX_NEVENT;
    ec->events = (struct epoll_event *)calloc(EPOLL_MAX_NEVENT, sizeof(struct epoll_event));
    if (!ec->events) {
        loge("malloc epoll_event failed!\n");
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
        loge("errno=%d %s\n", errno, strerror(errno));
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
            loge("epoll_wait failed %d: %s\n", errno, strerror(errno));
            return -1;
        }
        return 0;
    }
    if (0 == n) {
        loge("epoll_wait timeout\n");
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
    epoll_init,
    epoll_deinit,
    epoll_add,
    epoll_del,
    epoll_dispatch,
};
