/*****************************************************************************
 * Copyright (C) 2014-2015
 * file:    libthread.c
 * author:  gozfree <gozfree@163.com>
 * created: 2015-08-15 22:57
 * updated: 2016-01-03 15:38
 *****************************************************************************/
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <stdarg.h>
#include <libgzf.h>
#include <liblog.h>
#include "libthread.h"


/*
 * thread capability description must map to thread_cap
 */
const char *thread_cap_desc[] = {
    "thread_capability",
    "thread_create",
    "thread_destroy",
    "thread_cond_wait",
    "thread_cond_signal",
    "thread_mutex_lock",
    "thread_mutex_unlock",
    "thread_sem_lock",
    "thread_sem_unlock",
    "thread_sem_wait",
    "thread_sem_signal",
    "thread_print_info",
};


static void *__thread_func(void *arg)
{
    struct thread *t = (struct thread *)arg;
    if (t->func) {
        logd("thread %s is created, thread_id = %ld, thread_self = %ld\n",
             t->name, t->tid, pthread_self());
        t->is_run = 1;
        t->func(t, t->arg);
        t->is_run = 0;
        logd("thread %s exits\n", t->name);
    } else {
        logw("thread function is null\n");
    }

    return NULL;
}

#define PTHREAD_NAME_LEN    16

struct thread *thread_create(const char *name,
                void *(*func)(struct thread *, void *), void *arg)
{
    int ret;
    struct thread *t = CALLOC(1, struct thread);
    if (!t) {
        loge("malloc thread failed(%d): %s\n", errno, strerror(errno));
        goto err;
    }
    if (-1 == sem_init(&t->sem, 0, 0)) {
        loge("sem_init failed(%d): %s\n", errno, strerror(errno));
        goto err;
    }
    pthread_mutex_init(&t->mutex, NULL);
    if (0 != pthread_cond_init(&t->cond, NULL)) {
        loge("pthread_cond_init failed(%d): %s\n", errno, strerror(errno));
        goto err;
    }
    t->arg = arg;
    t->func = func;
    t->is_run = 0;
    if (name) {
        t->name = strdup(name);
    }
    if (0 != pthread_create(&t->tid, NULL, __thread_func, t)) {
        loge("pthread_create failed(%d): %s\n", errno, strerror(errno));
        goto err;
    }
    if (name) {
        if (0 != (ret = pthread_setname_np(t->tid, name))) {
            if (ret == ERANGE) {
                loge("thread name is out of range, should be less than %d\n",
                                PTHREAD_NAME_LEN);
            } else {
                loge("pthread_setname_np ret = %d, failed(%d): %s\n",
                                ret, errno, strerror(errno));
            }
            goto err;
        }
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
    pthread_mutex_destroy(&t->mutex);
    if (t->name) {
        free(t->name);
    }
    free(t);
}

int thread_cond_wait(struct thread *t)
{
    return pthread_cond_wait(&t->cond, &t->mutex);
}

int thread_cond_signal(struct thread *t)
{
    return pthread_cond_signal(&t->cond);
}

int thread_mutex_lock(struct thread *t)
{
    return pthread_mutex_lock(&t->mutex);
}

int thread_mutex_unlock(struct thread *t)
{
    return pthread_mutex_unlock(&t->mutex);
}

int thread_sem_wait(struct thread *t, int64_t ms)
{
    int ret;
    if (ms < 0) {
        ret = sem_wait(&t->sem);
    } else {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += ms / 1000;
        ts.tv_nsec += (ms % 1000) * 1000000;
        ret = sem_timedwait(&t->sem, &ts);
    }
    return ret;
}

int thread_sem_signal(struct thread *t)
{
    int ret;
    ret = sem_trywait(&t->sem);
    ret += sem_post(&t->sem);
    return ret;
}

void thread_print_info(struct thread *t)
{
    pthread_mutex_lock(&t->mutex);
    logi("========\n");
    logi("thread name = %s\n", t->name);
    logi("thread id = %ld\n", t->tid);
    logi("thread status=%s\n", t->is_run?"running":"stopped");
    logi("========\n");
    pthread_mutex_unlock(&t->mutex);
}

int thread_capability(struct capability_desc *desc)
{
    desc->entry = sizeof(thread_cap_desc)/sizeof(thread_cap_desc[0]);
    desc->cap = (char **)thread_cap_desc;
    return desc->entry;
}
