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
#include <gear-lib/libdarray.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/select.h>

#define SELECT_MAX_FD	1024

struct select_ctx {
    int nfds;		/* Highest fd in fd set */
    fd_set rfds;
    fd_set wfds;
    fd_set efds;
    DARRAY(struct gevent) ev_list;
};

static void *select_init(void)
{
    struct select_ctx *c = calloc(1, sizeof(struct select_ctx));
    if (!c) {
        printf("malloc select_ctx failed!\n");
        return NULL;
    }
    FD_ZERO(&c->rfds);
    FD_ZERO(&c->wfds);
    FD_ZERO(&c->efds);
    da_init(c->ev_list);
    return c;
}

static void select_deinit(void *ctx)
{
    struct select_ctx *c = (struct select_ctx *)ctx;
    if (c) {
        da_free(c->ev_list);
        free(c);
    }
}

static int select_add(struct gevent_base *eb, struct gevent *e)
{
    struct select_ctx *c = (struct select_ctx *)eb->ctx;

    if (c->nfds < e->evfd)
        c->nfds = e->evfd;

    if (e->flags & EVENT_READ)
        FD_SET(e->evfd, &c->rfds);
    if (e->flags & EVENT_WRITE)
        FD_SET(e->evfd, &c->wfds);
    if (e->flags & EVENT_EXCEPT)
        FD_SET(e->evfd, &c->efds);

    da_push_back(c->ev_list, e);

    return 0;
}

static int select_del(struct gevent_base *eb, struct gevent *e)
{
    struct select_ctx *c = (struct select_ctx *)eb->ctx;
    FD_CLR(e->evfd, &c->rfds);
    FD_CLR(e->evfd, &c->wfds);
    FD_CLR(e->evfd, &c->efds);
    da_erase_item(c->ev_list, e);
    return 0;
}

static int select_dispatch(struct gevent_base *eb, struct timeval *tv)
{
    int i, n;
    struct select_ctx *c = (struct select_ctx *)eb->ctx;
    int nfds = c->nfds + 1;

    n = select(nfds, &c->rfds, &c->wfds, &c->efds, tv);
    if (-1 == n) {
        printf("errno=%d %s\n", errno, strerror(errno));
        return -1;
    }
    if (0 == n) {
        printf("select timeout\n");
        return 0;
    }
    for (i = 0; i < c->ev_list.num; i++) {
        struct gevent *e = &c->ev_list.array[i];
        if (FD_ISSET(e->evfd, &c->rfds) && e->evcb->ev_in) {
            e->evcb->ev_in(e->evfd, e->evcb->args);
        }
        if (FD_ISSET(e->evfd, &c->wfds) && e->evcb->ev_out) {
            e->evcb->ev_out(e->evfd, e->evcb->args);
        }
        if (FD_ISSET(e->evfd, &c->efds) && e->evcb->ev_err) {
            e->evcb->ev_err(e->evfd, e->evcb->args);
        }
    }
    return 0;
}

struct gevent_ops selectops = {
    .init     = select_init,
    .deinit   = select_deinit,
    .add      = select_add,
    .del      = select_del,
    .dispatch = select_dispatch,
};
