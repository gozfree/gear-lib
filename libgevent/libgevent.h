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
#ifndef LIBGEVENT_H
#define LIBGEVENT_H

#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

enum gevent_flags {
    EVENT_TIMEOUT  = 1<<0,
    EVENT_READ     = 1<<1,
    EVENT_WRITE    = 1<<2,
    EVENT_SIGNAL   = 1<<3,
    EVENT_PERSIST  = 1<<4,
    EVENT_ET       = 1<<5,
    EVENT_FINALIZE = 1<<6,
    EVENT_CLOSED   = 1<<7,
    EVENT_ERROR    = 1<<8,
    EVENT_EXCEPT   = 1<<9,
};

struct gevent_cbs {
    void (*ev_in)(int fd, void *arg);
    void (*ev_out)(int fd, void *arg);
    void (*ev_err)(int fd, void *arg);
    void *args;
};

struct gevent {
    int evfd;
    int flags;
    struct gevent_cbs *evcb;
};

struct gevent_base;
struct gevent_ops {
    void *(*init)();
    void (*deinit)(void *ctx);
    int (*add)(struct gevent_base *eb, struct gevent *e);
    int (*del)(struct gevent_base *eb, struct gevent *e);
    int (*dispatch)(struct gevent_base *eb, struct timeval *tv);
};

struct gevent_base {
    /** Pointer to backend-specific data. */
    void *ctx;
    int loop;
    int rfd;
    int wfd;
    const struct gevent_ops *evop;
};

struct gevent_base *gevent_base_create();
void gevent_base_destroy(struct gevent_base *);
int gevent_base_loop(struct gevent_base *);
void gevent_base_loop_break(struct gevent_base *);
int gevent_base_wait(struct gevent_base *eb);
void gevent_base_signal(struct gevent_base *eb);

struct gevent *gevent_create(int fd,
        void (ev_in)(int, void *),
        void (ev_out)(int, void *),
        void (ev_err)(int, void *),
        void *args);

void gevent_destroy(struct gevent *e);
int gevent_add(struct gevent_base *eb, struct gevent *e);
int gevent_del(struct gevent_base *eb, struct gevent *e);

#ifdef __cplusplus
}
#endif
#endif
