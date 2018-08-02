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
