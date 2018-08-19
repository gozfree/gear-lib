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
#include "libthread.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <stdarg.h>

#define CALLOC(size, type) \
    (type *)calloc(size, sizeof(type))


static void *__thread_func(void *arg)
{
    struct thread *t = (struct thread *)arg;
    if (!t->func) {
        printf("thread function is null\n");
        return NULL;
    }
    t->func(t, t->arg);
    return NULL;
}

struct thread *thread_create(void *(*func)(struct thread *, void *), void *arg, ...)
{
    struct thread *t = CALLOC(1, struct thread);
    if (!t) {
        printf("malloc thread failed(%d): %s\n", errno, strerror(errno));
        goto err;
    }
    enum lock_type type;
    va_list ap;
    va_start(ap, arg);
    type = va_arg(ap, enum lock_type);
    va_end(ap);

    if (type != LOCK_SPIN && type != LOCK_MUTEX &&
        type != LOCK_SEM && type != LOCK_COND) {
        type = LOCK_COND;//default
    }
    t->type = type;

    if (0 != pthread_attr_init(&t->attr)) {
        printf("pthread_attr_init() failed\n");
        goto err;
    }

    switch (type) {
    case LOCK_SPIN:
        break;
    case LOCK_MUTEX:
        if (0 != mutex_lock_init(&t->lock.mutex)) {
            printf("mutex_lock_init failed\n");
            goto err;
        }
        break;
    case LOCK_SEM:
        if (0 != sem_lock_init(&t->lock.sem)) {
            printf("sem_lock_init failed\n");
            goto err;
        }
        break;
    case LOCK_COND:
        if (0 != mutex_cond_init(&t->lock.cond)) {
            printf("mutex_cond_init failed\n");
            goto err;
        }
        break;
    case LOCK_RW:
        break;
    default:
        break;
    }

    t->arg = arg;
    t->func = func;
    t->run = true;
    if (0 != pthread_create(&t->tid, &t->attr, __thread_func, t)) {
        printf("pthread_create failed(%d): %s\n", errno, strerror(errno));
        goto err;
    }
    return t;

err:
    if (t) {
        free(t);
    }
    return NULL;
}

void thread_destroy(struct thread *t)
{
    if (!t) {
        return;
    }
    t->run = false;
    switch (t->type) {
    case LOCK_SPIN:
        break;
    case LOCK_SEM:
        sem_lock_deinit(&t->lock.sem);
        break;
    case LOCK_MUTEX:
        mutex_lock_deinit(&t->lock.mutex);
        break;
    case LOCK_COND:
        mutex_cond_signal_all(&t->lock.cond);
        mutex_lock_deinit(&t->lock.mutex);
        mutex_cond_deinit(&t->lock.cond);
        break;
    default:
        break;
    }
    pthread_join(t->tid, NULL);
    pthread_attr_destroy(&t->attr);
    free(t);
}

void thread_info(struct thread *t)
{
    int i;
    size_t v;
    void *stkaddr;
    struct sched_param sp;

    printf("thread attribute info:\n");
    if (0 == pthread_attr_getdetachstate(&t->attr, &i)) {
        printf("detach state = %s\n",
            (i == PTHREAD_CREATE_DETACHED) ? "PTHREAD_CREATE_DETACHED" :
            (i == PTHREAD_CREATE_JOINABLE) ? "PTHREAD_CREATE_JOINABLE" :
            "???");
    }
    if (0 == pthread_attr_getscope(&t->attr, &i)) {
        printf("scope = %s\n",
            (i == PTHREAD_SCOPE_SYSTEM) ? "PTHREAD_SCOPE_SYSTEM" :
            (i == PTHREAD_SCOPE_PROCESS) ? "PTHREAD_SCOPE_PROCESS" :
            "???");
    }
    if (0 == pthread_attr_getinheritsched(&t->attr, &i)) {
        printf("inherit scheduler = %s\n",
            (i == PTHREAD_INHERIT_SCHED) ? "PTHREAD_INHERIT_SCHED" :
            (i == PTHREAD_EXPLICIT_SCHED) ? "PTHREAD_EXPLICIT_SCHED" :
            "???");
    }
    if (0 == pthread_attr_getschedpolicy(&t->attr, &i)) {
        printf("scheduling policy = %s\n",
            (i == SCHED_OTHER) ? "SCHED_OTHER" :
            (i == SCHED_FIFO) ? "SCHED_FIFO" :
            (i == SCHED_RR) ? "SCHED_RR" :
            "???");
    }

    if (0 == pthread_attr_getschedparam(&t->attr, &sp)) {
        printf("scheduling priority = %d\n", sp.sched_priority);
    }

    if (0 == pthread_attr_getguardsize(&t->attr, &v)) {
        printf("guard size = %zu bytes\n", v);
    }

    if (0 == pthread_attr_getstack(&t->attr, &stkaddr, &v)) {
        printf("stack address = %p, size = %zu\n", stkaddr, v);
    }
}

int thread_lock(struct thread *t)
{
    if (!t) {
        return -1;
    }
    switch (t->type) {
    case LOCK_MUTEX:
        return mutex_lock(&t->lock.mutex);
        break;
    case LOCK_SPIN:
        return spin_lock(&t->lock.spin);
        break;
    default:
        break;
    }
    return -1;
}

int thread_unlock(struct thread *t)
{
    if (!t) {
        return -1;
    }
    switch (t->type) {
    case LOCK_MUTEX:
        return mutex_unlock(&t->lock.mutex);
        break;
    case LOCK_SPIN:
        return spin_unlock(&t->lock.spin);
        break;
    default:
        break;
    }
    return -1;
}

int thread_wait(struct thread *t, int64_t ms)
{
    if (!t) {
        return -1;
    }
    switch (t->type) {
    case LOCK_COND:
    case LOCK_MUTEX:
        return mutex_cond_wait(&t->lock.mutex, &t->lock.cond, ms);
        break;
    case LOCK_SEM:
        return sem_lock_wait(&t->lock.sem, ms);
        break;
    default:
        break;
    }
    return -1;
}

int thread_signal(struct thread *t)
{
    if (!t) {
        return -1;
    }
    switch (t->type) {
    case LOCK_COND:
        mutex_cond_signal(&t->lock.cond);
        break;
    case LOCK_SEM:
        return sem_lock_signal(&t->lock.sem);
        break;
    default:
        break;
    }
    return 0;
}

int thread_signal_all(struct thread *t)
{
    if (!t) {
        return -1;
    }
    switch (t->type) {
    case LOCK_COND:
        mutex_cond_signal_all(&t->lock.cond);
        break;
    default:
        break;
    }
    return 0;
}
