/*****************************************************************************
 * Copyright (C) 2014-2015
 * file:    select.c
 * author:  gozfree <gozfree@163.com>
 * created: 2015-04-27 00:59
 * updated: 2015-07-12 00:41
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/select.h>
#include <libgzf.h>
#include <liblog.h>
#include "libgevent.h"

#define SELECT_MAX_FD	1024

struct select_ctx {
    int nfds;		/* Highest fd in fd set */
    fd_set *rfds;
    fd_set *wfds;
    fd_set *efds;
};

static void *select_init()
{
    struct select_ctx *sc = CALLOC(1, struct select_ctx);
    if (!sc) {
        loge("malloc select_ctx failed!\n");
        return NULL;
    }
    fd_set *rfds = CALLOC(1, fd_set);
    fd_set *wfds = CALLOC(1, fd_set);
    fd_set *efds = CALLOC(1, fd_set);
    if (!rfds || !wfds || !efds) {
        loge("malloc fd_set failed!\n");
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
        loge("errno=%d %s\n", errno, strerror(errno));
        return -1;
    }
    if (0 == n) {
        loge("select timeout\n");
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
    select_init,
    select_deinit,
    select_add,
    select_del,
    select_dispatch,
};
