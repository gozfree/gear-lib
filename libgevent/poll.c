/*******************************************************************************
 * Copyright (C) 2014-2015
 * file:    poll.c
 * author:  Zhifeng Gong <gozfree@163.com>
 * created: 2015-04-27 00:59
 * updated: 2015-07-12 00:41
 ******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>
#include <poll.h>
#include "libgzf.h"
#include "libgevent.h"

#define POLL_MAX_FD	1024
#define MAX_SECONDS_IN_MSEC_LONG \
	(((LONG_MAX) - 999) / 1000)


struct poll_ctx {
    nfds_t nfds;
    int event_count;
    struct pollfd *fds;
};

static void *poll_init()
{
    struct poll_ctx *pc;
    struct pollfd *fds;
    pc = (struct poll_ctx *)calloc(1, sizeof(struct poll_ctx));
    if (!pc) {
        printf("malloc poll_ctx failed!\n");
        return NULL;
    }
    fds = (struct pollfd *)calloc(POLL_MAX_FD, sizeof(struct pollfd));
    if (!fds) {
        printf("malloc pollfd failed!\n");
        return NULL;
    }
    pc->fds = fds;

    return pc;
}

static void poll_deinit(void *ctx)
{
    struct poll_ctx *pc = (struct poll_ctx *)ctx;
    if (!ctx) {
        return;
    }
    free(pc->fds);
    free(pc);
}

static int poll_add(struct gevent_base *eb, struct gevent *e)
{
    struct poll_ctx *pc = eb->ctx;

    pc->fds[0].fd = e->evfd;

    if (e->flags & EVENT_READ)
        pc->fds[0].events |= POLLIN;
    if (e->flags & EVENT_WRITE)
        pc->fds[0].events |= POLLOUT;
    if (e->flags & EVENT_EXCEPT)
        pc->fds[0].events |= POLLERR;

    return 0;
}
static int poll_del(struct gevent_base *eb, struct gevent *e)
{
//    struct poll_ctx *pc = eb->base;

    return 0;
}
static int poll_dispatch(struct gevent_base *eb, struct timeval *tv)
{
    struct poll_ctx *pc = eb->ctx;
    int i, n;
    int flags;
    int timeout = -1;

    if (tv != NULL) {
        if (tv->tv_usec > 1000000 || tv->tv_sec > MAX_SECONDS_IN_MSEC_LONG)
            timeout = -1;
        else
            timeout = (tv->tv_sec * 1000) + ((tv->tv_usec + 999) / 1000);
    } else {
        timeout = -1;
    }

    n = poll(pc->fds, pc->nfds, timeout);
    if (-1 == n) {
        printf("errno=%d %s\n", errno, strerror(errno));
        return -1;
    }
    if (0 == n) {
        printf("poll timeout\n");
        return 0;
    }
    for (i = 0; i < n; i++) {
        if (pc->fds[i].revents & POLLIN)
            flags |= EVENT_READ;
        if (pc->fds[i].revents & POLLOUT)
            flags |= EVENT_WRITE;
        if (pc->fds[i].revents & (POLLERR|POLLHUP|POLLNVAL))
            flags |= EVENT_EXCEPT;
        pc->fds[i].revents = 0;
    }
    return 0;
}

const struct gevent_ops pollops = {
	poll_init,
	poll_deinit,
	poll_add,
	poll_del,
	poll_dispatch,
};
