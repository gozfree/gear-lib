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
#include <libmacro.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/select.h>

#define SELECT_MAX_FD	1024

struct select_ctx {
    int nfds;		/* Highest fd in fd set */
    fd_set *rfds;
    fd_set *wfds;
    fd_set *efds;
};

static void *select_init(void)
{
    struct select_ctx *sc = CALLOC(1, struct select_ctx);
    if (!sc) {
        printf("malloc select_ctx failed!\n");
        return NULL;
    }
    fd_set *rfds = CALLOC(1, fd_set);
    fd_set *wfds = CALLOC(1, fd_set);
    fd_set *efds = CALLOC(1, fd_set);
    if (!rfds || !wfds || !efds) {
        printf("malloc fd_set failed!\n");
        return NULL;
    }
    sc->rfds = rfds;
    sc->wfds = wfds;
    sc->efds = efds;
    return sc;
}

static void select_deinit(void *ctx)
{
    struct select_ctx *sc = (struct select_ctx *)ctx;
    if (!ctx) {
        return;
    }
    free(sc->rfds);
    free(sc->wfds);
    free(sc->efds);
    free(sc);
}

static int select_add(struct gevent_base *eb, struct gevent *e)
{
    struct select_ctx *sc = (struct select_ctx *)eb->ctx;

    FD_ZERO(sc->rfds);
    FD_ZERO(sc->wfds);
    FD_ZERO(sc->efds);

    if (sc->nfds < e->evfd) {
        sc->nfds = e->evfd;
    }

    if (e->flags & EVENT_READ)
        FD_SET(e->evfd, sc->rfds);
    if (e->flags & EVENT_WRITE)
        FD_SET(e->evfd, sc->wfds);
    if (e->flags & EVENT_EXCEPT)
        FD_SET(e->evfd, sc->efds);
    return 0;
}

static int select_del(struct gevent_base *eb, struct gevent *e)
{
    struct select_ctx *sc = (struct select_ctx *)eb->ctx;
    if (sc->rfds)
        FD_CLR(e->evfd, sc->rfds);
    if (sc->wfds)
        FD_CLR(e->evfd, sc->wfds);
    if (sc->efds)
        FD_CLR(e->evfd, sc->efds);
    return 0;
}

static int select_dispatch(struct gevent_base *eb, struct timeval *tv)
{
    int i, n;
    int flags;
    struct select_ctx *sc = (struct select_ctx *)eb->ctx;
    int nfds = sc->nfds + 1;

    n = select(nfds, sc->rfds, sc->wfds, sc->efds, tv);
    if (-1 == n) {
        printf("errno=%d %s\n", errno, strerror(errno));
        return -1;
    }
    if (0 == n) {
        printf("select timeout\n");
        return 0;
    }
    for (i = 0; i < nfds; i++) {
        if (FD_ISSET(i, sc->rfds))
            flags |= EVENT_READ;
        if (FD_ISSET(i, sc->wfds))
            flags |= EVENT_WRITE;
        if (FD_ISSET(i, sc->efds))
            flags |= EVENT_EXCEPT;
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
