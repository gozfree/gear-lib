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
#include "libgevent.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>
#include <poll.h>

#define POLL_MAX_FD                 (1024)
#define MAX_SECONDS_IN_MSEC_LONG    (((LONG_MAX) - 999) / 1000)


struct poll_ctx {
    nfds_t nfds;
    int event_count;
    struct pollfd *fds;
};

static void *poll_init(void)
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
    struct poll_ctx *pc = (struct poll_ctx *)eb->ctx;

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
    struct poll_ctx *pc = (struct poll_ctx *)eb->ctx;
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

struct gevent_ops pollops = {
    .init     = poll_init,
    .deinit   = poll_deinit,
    .add      = poll_add,
    .del      = poll_del,
    .dispatch = poll_dispatch,
};
