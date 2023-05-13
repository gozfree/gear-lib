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
#if defined (__linux__) || defined (__CYGWIN__)
/*NOTE: must be firstly */
#define _GNU_SOURCE
#include <pthread.h>
#endif
#include "libthread.h"
#include "libatomic.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>


static void *__thread_func(void *arg)
{
    struct thread *t = (struct thread *)arg;
    if (!t->func) {
        printf("thread function is null\n");
        return NULL;
    }
    t->run = true;
    t->func(t, t->arg);
    t->run = false;
    return NULL;
}

struct thread *thread_create(void *(*func)(struct thread *, void *), void *arg)
{
    enum lock_type type = THREAD_LOCK_COND;
    struct thread *t = CALLOC(1, struct thread);
    if (!t) {
        printf("malloc thread failed(%d): %s\n", errno, strerror(errno));
        goto err;
    }

    if (type != THREAD_LOCK_SPIN && type != THREAD_LOCK_MUTEX &&
        type != THREAD_LOCK_SEM && type != THREAD_LOCK_COND) {
        type = THREAD_LOCK_COND;//default
    }
    t->type = type;

    if (0 != pthread_attr_init(&t->attr)) {
        printf("pthread_attr_init() failed\n");
        goto err;
    }

    switch (type) {
    case THREAD_LOCK_SPIN:
        break;
    case THREAD_LOCK_MUTEX:
        if (0 != mutex_lock_init(&t->lock.mutex)) {
            printf("mutex_lock_init failed\n");
            goto err;
        }
        break;
    case THREAD_LOCK_SEM:
        if (0 != sem_lock_init(&t->lock.sem)) {
            printf("sem_lock_init failed\n");
            goto err;
        }
        break;
    case THREAD_LOCK_COND:
        if (0 != mutex_cond_init(&t->cond)) {
            printf("mutex_cond_init failed\n");
            goto err;
        }
        break;
    case THREAD_LOCK_RW:
        break;
    default:
        break;
    }

    t->arg = arg;
    t->func = func;
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

int thread_join(struct thread *t)
{
    if (!t) {
        return -1;
    }
    return pthread_join(t->tid, NULL);
}

void thread_destroy(struct thread *t)
{
    if (!t) {
        return;
    }
    switch (t->type) {
    case THREAD_LOCK_SPIN:
        break;
    case THREAD_LOCK_SEM:
        sem_lock_deinit(&t->lock.sem);
        break;
    case THREAD_LOCK_MUTEX:
        mutex_lock_deinit(&t->lock.mutex);
        break;
    case THREAD_LOCK_COND:
        mutex_cond_signal_all(&t->cond);
        mutex_lock_deinit(&t->lock.mutex);
        mutex_cond_deinit(&t->cond);
        break;
    default:
        break;
    }
    pthread_attr_destroy(&t->attr);
    free(t);
}

int thread_set_name(struct thread *t, const char *name)
{
#if defined (OS_LINUX) || defined (OS_WINDOWS)
    if (0 != pthread_setname_np(t->tid, name)) {
        printf("pthread_setname_np %s failed: %d\n", name, errno);
        return -1;
    }
    strncpy(t->name, name, sizeof(t->name));
    return 0;
#else
    return 0;
#endif
}

void thread_get_info(struct thread *t)
{
#if defined (OS_LINUX) || defined (OS_WINDOWS)
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
#if !defined (OS_WINDOWS)
    if (0 == pthread_attr_getguardsize(&t->attr, &v)) {
        printf("guard size = %zu bytes\n", v);
    }

    if (0 == pthread_attr_getstack(&t->attr, &stkaddr, &v)) {
        printf("stack address = %p, size = %zu\n", stkaddr, v);
    }
#endif

    if (0 == pthread_getname_np(t->tid, t->name, sizeof(t->name))) {
        printf("thread name = %s\n", t->name);
    }
#endif
}

int thread_lock(struct thread *t)
{
    if (!t) {
        return -1;
    }
    switch (t->type) {
    case THREAD_LOCK_MUTEX:
        return mutex_lock(&t->lock.mutex);
        break;
    case THREAD_LOCK_SPIN:
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
    case THREAD_LOCK_MUTEX:
        return mutex_unlock(&t->lock.mutex);
        break;
    case THREAD_LOCK_SPIN:
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
        printf("%s invalid paramenters!\n", __func__);
        return -1;
    }
    switch (t->type) {
    case THREAD_LOCK_COND:
    case THREAD_LOCK_MUTEX:
        return mutex_cond_wait(&t->lock.mutex, &t->cond, ms);
        break;
    case THREAD_LOCK_SEM:
        return sem_lock_wait(&t->lock.sem, ms);
        break;
    default:
        printf("%s unsupport thread type!\n", __func__);
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
    case THREAD_LOCK_COND:
        mutex_cond_signal(&t->cond);
        break;
    case THREAD_LOCK_SEM:
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
    case THREAD_LOCK_COND:
        mutex_cond_signal_all(&t->cond);
        break;
    default:
        break;
    }
    return 0;
}
