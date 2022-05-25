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
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#if defined (OS_LINUX) || defined (OS_APPLE)
#ifndef __CYGWIN__
#include <sys/eventfd.h>
#endif
#endif

#if defined (OS_LINUX) || defined (OS_RTTHREAD) || defined (OS_RTOS) || defined (OS_APPLE)
extern const struct gevent_ops selectops;
extern const struct gevent_ops pollops;
#endif
#if defined (OS_LINUX) || defined (OS_WINDOWS)
#ifndef __CYGWIN__
extern const struct gevent_ops epollops;
#endif
#endif
#if defined (OS_WINDOWS)
extern const struct gevent_ops iocpops;
#endif

enum gevent_backend_type {
#if defined (OS_LINUX) || defined (OS_RTTHREAD) || defined (OS_RTOS) || defined (OS_APPLE)
    GEVENT_SELECT,
    GEVENT_POLL,
#endif
#if defined (OS_LINUX) || defined (OS_WINDOWS)
    GEVENT_EPOLL,
#endif
#if defined (OS_WINDOWS)
    GEVENT_IOCP,
#endif
};

struct gevent_backend {
    enum gevent_backend_type type;
    const struct gevent_ops *ops;
};

static struct gevent_backend gevent_backend_list[] = {
#if defined (OS_LINUX) || defined (OS_RTTHREAD) || defined (OS_RTOS) || defined (OS_APPLE)
    {GEVENT_SELECT, &selectops},
    {GEVENT_POLL,   &pollops},
#endif
#if defined (OS_LINUX) || defined (OS_WINDOWS)
#ifndef __CYGWIN__
    {GEVENT_EPOLL,  &epollops},
#endif
#endif
#if defined (OS_WINDOWS)
    {GEVENT_IOCP,   &iocpops},
#endif
};

#if defined (OS_LINUX)
#define GEVENT_BACKEND GEVENT_EPOLL
#elif defined (OS_APPLE)
#define GEVENT_BACKEND GEVENT_POLL
#elif defined (OS_WINDOWS)
//#define GEVENT_BACKEND GEVENT_IOCP
#define GEVENT_BACKEND GEVENT_EPOLL
#else
#define GEVENT_BACKEND GEVENT_POLL
#endif

static void event_in(int fd, void *arg)
{
    uint64_t notify;
    if (sizeof(uint64_t) != read(fd, &notify, sizeof(uint64_t))) {
        printf("read notify failed %d\n", errno);
    }
}

struct gevent_base *gevent_base_create(void)
{
    struct gevent_base *eb = NULL;
    eb = (struct gevent_base *)calloc(1, sizeof(struct gevent_base));
    if (!eb) {
        printf("malloc gevent_base failed!\n");
        return NULL;
    }

    eb->ops = gevent_backend_list[GEVENT_BACKEND].ops;
    if (!eb->ops) {
        printf("gevent_backend_list ops is invalid!\n");
        goto failed;
    }
    eb->ctx = eb->ops->init();

    eb->loop = 1;
    eb->inner_fd = eventfd(0, 0);
    if (eb->inner_fd == -1) {
        printf("eventfd failed %d\n", errno);
        goto failed;
    }
    da_init(eb->ev_array);
    eb->inner_event = gevent_create(eb->inner_fd, event_in, NULL, NULL, NULL);
    if (!eb->inner_event) {
        printf("gevent_create inner_event failed!\n");
        goto failed;
    }
    gevent_add(eb, &eb->inner_event);
    return eb;

failed:
    if (eb) {
        free(eb);
    }
    return NULL;
}

void gevent_base_destroy(struct gevent_base *eb)
{
    if (!eb) {
        return;
    }
    if (eb->loop) {
        gevent_base_loop_break(eb);
    }
    gevent_del(eb, &eb->inner_event);
    gevent_destroy(eb->inner_event);
    close(eb->inner_fd);
    eb->ops->deinit(eb->ctx);
    while (eb->ev_array.num > 0) {
        struct gevent **e = eb->ev_array.array + eb->ev_array.num-1;
        da_erase_item(eb->ev_array, e);
        free(*e);
    }
    da_free(eb->ev_array);
    free(eb);
}

int gevent_base_wait(struct gevent_base *eb)
{
    const struct gevent_ops *ops = eb->ops;
    return ops->dispatch(eb, NULL);
}

int gevent_base_loop(struct gevent_base *eb)
{
    const struct gevent_ops *ops = eb->ops;
    int ret;
    while (eb->loop) {
        ret = ops->dispatch(eb, NULL);
        if (ret == -1) {
            printf("dispatch failed\n");
        }
    }
    return 0;
}

static void *_gevent_base_loop(struct thread *t, void *arg)
{
    struct gevent_base *eb = (struct gevent_base *)arg;
    gevent_base_loop(eb);
    return NULL;
}

int gevent_base_loop_start(struct gevent_base *eb)
{
    eb->thread = thread_create(_gevent_base_loop, eb);
    return 0;
}

int gevent_base_loop_stop(struct gevent_base *eb)
{
    gevent_base_loop_break(eb);
    thread_join(eb->thread);
    thread_destroy(eb->thread);
    return 0;
}

void gevent_base_loop_break(struct gevent_base *eb)
{
    uint64_t notify = '1';
    eb->loop = 0;
    if (sizeof(uint64_t) != write(eb->inner_fd, &notify, sizeof(uint64_t))) {
        perror("write error");
    }
}

void gevent_base_signal(struct gevent_base *eb)
{
    uint64_t notify = '1';
    if (sizeof(uint64_t) != write(eb->inner_fd, &notify, sizeof(uint64_t))) {
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
    struct gevent *e = (struct gevent *)calloc(1, sizeof(struct gevent));
    if (!e) {
        printf("malloc gevent failed!\n");
        return NULL;
    }
    e->evcb.ev_in = ev_in;
    e->evcb.ev_out = ev_out;
    e->evcb.ev_err = ev_err;
    e->evcb.ev_timer = NULL;
    e->evcb.args = args;
    if (ev_in) {
        flags |= EVENT_READ;
    }
    if (ev_out) {
        flags |= EVENT_WRITE;
    }
    if (ev_err) {
        flags |= EVENT_ERROR;
    }

    flags |= EVENT_PERSIST;
    e->evfd = fd;
    e->flags = flags;

    return e;
}

void gevent_timer_destroy(struct gevent *e)
{
    if (!e) {
        return;
    }
    if (e->evfd)
        close(e->evfd);
    if (e)
        free(e);
}

struct gevent *gevent_timer_create(time_t msec,
        enum gevent_timer_type type,
        void (ev_timer)(int, void *),
        void *args)
{
#if defined (OS_LINUX)
    enum gevent_flags flags = 0;
    int fd;
    time_t sec = msec/1000;
    long nsec = (msec-sec*1000)*1000000;

    struct gevent *e = (struct gevent *)calloc(1, sizeof(struct gevent));
    if (!e) {
        printf("malloc gevent failed!\n");
        goto failed;
    }

    e->evcb.ev_timer = ev_timer;
    e->evcb.ev_in = NULL;
    e->evcb.ev_out = NULL;
    e->evcb.ev_err = NULL;
    e->evcb.args = args;
    flags = EVENT_READ;
    if (type == TIMER_PERSIST) {
        flags |= EVENT_PERSIST;
    } else if (type == TIMER_ONESHOT) {
        flags &= ~EVENT_PERSIST;
    }

    fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
    if (fd == -1) {
        printf("timerfd_create failed %d\n", errno);
        goto failed;
    }

    e->evcb.itimer.it_value.tv_sec = sec;
    e->evcb.itimer.it_value.tv_nsec = nsec;
    e->evcb.itimer.it_interval.tv_sec = sec;
    e->evcb.itimer.it_interval.tv_nsec = nsec;
    if (0 != timerfd_settime(fd, 0, &e->evcb.itimer, NULL)) {
        printf("timerfd_settime failed!\n");
        goto failed;
    }

    e->evfd = fd;
    e->flags = flags;
    return e;

failed:
    if (e) free(e);
#endif
    return NULL;
}

void gevent_destroy(struct gevent *e)
{
    if (!e)
        return;
    free(e);
}

int gevent_add(struct gevent_base *eb, struct gevent **e)
{
    if (!e || !eb) {
        printf("%s:%d paraments is NULL\n", __func__, __LINE__);
        return -1;
    }
    da_push_back(eb->ev_array, e);
    return eb->ops->add(eb, *e);
}

int gevent_del(struct gevent_base *eb, struct gevent **e)
{
    int ret;
    if (!e || !eb) {
        printf("%s:%d paraments is NULL\n", __func__, __LINE__);
        return -1;
    }
    ret = eb->ops->del(eb, *e);
    da_erase_item(eb->ev_array, e);
    return ret;
}

int gevent_mod(struct gevent_base *eb, struct gevent **e)
{
    if (!e || !eb) {
        printf("%s:%d paraments is NULL\n", __func__, __LINE__);
        return -1;
    }
    da_erase_item(eb->ev_array, e);
    da_push_back(eb->ev_array, e);
    return eb->ops->mod(eb, *e);
}
