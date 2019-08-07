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
#ifndef LIBGEVENT_H
#define LIBGEVENT_H

#include <stdint.h>
#include <stdlib.h>
#if defined (__linux__) || defined (__CYGWIN__)
#include <pthread.h>
#include <sys/timerfd.h>
#elif defined (__WIN32__) || defined (WIN32) || defined (_MSC_VER)
#include "libposix4win.h"
#endif


#ifdef __cplusplus
extern "C" {
#endif

enum gevent_timer_type {
    TIMER_ONESHOT = 0,
    TIMER_PERSIST,
};

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
    void (*ev_timer)(int fd, void *arg);
#if defined (__linux__) || defined (__CYGWIN__)
    struct itimerspec itimer;
#endif
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
    pthread_t tid;
    const struct gevent_ops *evop;
};

struct gevent_base *gevent_base_create();
void gevent_base_destroy(struct gevent_base *);
int gevent_base_loop(struct gevent_base *);
int gevent_base_loop_start(struct gevent_base *eb);
int gevent_base_loop_stop(struct gevent_base *eb);
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
struct gevent *gevent_timer_create(time_t msec,
        enum gevent_timer_type type,
        void (ev_timer)(int, void *),
        void *args);

#ifdef __cplusplus
}
#endif
#endif
