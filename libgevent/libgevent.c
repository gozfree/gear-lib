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
#include <unistd.h>
#include <fcntl.h>

extern const struct gevent_ops selectops;
extern const struct gevent_ops pollops;
extern const struct gevent_ops epollops;

static const struct gevent_ops *eventops[] = {
//    &selectops,
    &epollops,
    NULL
};

static void event_in(int fd, void *arg)
{
}

struct gevent_base *gevent_base_create(void)
{
    int i;
    int fds[2];
    struct gevent_base *eb = NULL;
    if (pipe(fds)) {
        perror("pipe failed");
        return NULL;
    }
    eb = CALLOC(1, struct gevent_base);
    if (!eb) {
        printf("malloc gevent_base failed!\n");
        close(fds[0]);
        close(fds[1]);
        return NULL;
    }

    for (i = 0; eventops[i]; i++) {
        eb->ctx = eventops[i]->init();
        eb->evop = eventops[i];
    }
    eb->loop = 1;
    eb->rfd = fds[0];
    eb->wfd = fds[1];
    fcntl(fds[0], F_SETFL, fcntl(fds[0], F_GETFL) | O_NONBLOCK);
    struct gevent *e = gevent_create(eb->rfd, event_in, NULL, NULL, NULL);
    gevent_add(eb, e);
    return eb;
}

void gevent_base_destroy(struct gevent_base *eb)
{
    if (!eb) {
        return;
    }
    gevent_base_loop_break(eb);
    close(eb->rfd);
    close(eb->wfd);
    eb->evop->deinit(eb->ctx);
    free(eb);
}

int gevent_base_loop(struct gevent_base *eb)
{
    const struct gevent_ops *evop = eb->evop;
    int ret;
    while (eb->loop) {
        ret = evop->dispatch(eb, NULL);
        if (ret == -1) {
            printf("dispatch failed\n");
//            return -1;
        }
    }

    return 0;
}

int gevent_base_wait(struct gevent_base *eb)
{
    const struct gevent_ops *evop = eb->evop;
    return evop->dispatch(eb, NULL);
}

void gevent_base_loop_break(struct gevent_base *eb)
{
    char buf[1];
    buf[0] = 0;
    eb->loop = 0;
    if (1 != write(eb->wfd, buf, 1)) {
        perror("write error");
    }
}

void gevent_base_signal(struct gevent_base *eb)
{
    char buf[1];
    buf[0] = 0;
    if (1 != write(eb->wfd, buf, 1)) {
        perror("write error");
    }
}

struct gevent *gevent_create(int fd,
        void (ev_in)(int, void *),
        void (ev_out)(int, void *),
        void (ev_err)(int, void *),
        void *args)
{
    int flags = 0;
    struct gevent *e = CALLOC(1, struct gevent);
    if (!e) {
        printf("malloc gevent failed!\n");
        return NULL;
    }
    struct gevent_cbs *evcb = CALLOC(1, struct gevent_cbs);
    if (!evcb) {
        printf("malloc gevent failed!\n");
        return NULL;
    }
    evcb->ev_in = ev_in;
    evcb->ev_out = ev_out;
    evcb->ev_err = ev_err;
    evcb->args = args;
    if (ev_in) {
        flags |= EVENT_READ;
    }
    if (ev_out) {
        flags |= EVENT_WRITE;
    }
    if (ev_err) {
        flags |= EVENT_ERROR;
    }

    e->evfd = fd;
    e->flags = flags;
    e->evcb = evcb;

    return e;
}

void gevent_destroy(struct gevent *e)
{
    if (!e)
        return;
    if (e->evcb)
        free(e->evcb);
    free(e);
}

int gevent_add(struct gevent_base *eb, struct gevent *e)
{
    if (!e || !eb) {
        printf("%s:%d paraments is NULL\n", __func__, __LINE__);
        return -1;
    }
    return eb->evop->add(eb, e);
}

int gevent_del(struct gevent_base *eb, struct gevent *e)
{
    if (!e || !eb) {
        printf("%s:%d paraments is NULL\n", __func__, __LINE__);
        return -1;
    }
    return eb->evop->del(eb, e);
}
