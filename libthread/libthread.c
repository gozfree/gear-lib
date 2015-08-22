/*****************************************************************************
 * Copyright (C) 2014-2015
 * file:    libthread.c
 * author:  gozfree <gozfree@163.com>
 * created: 2015-08-15 22:57
 * updated: 2015-08-15 22:57
 *****************************************************************************/
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <stdarg.h>
#include <libgzf.h>
#include <libglog.h>
#include "libthread.h"



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
    }

    return NULL;
}

struct thread *thread_create(const char *name, void *(*func)(struct thread *, void *), void *arg)
{
    struct thread *t = CALLOC(1, struct thread);
    if (!t) {
        loge("malloc thread failed(%d): %s\n", errno, strerror(errno));
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
        if (0 != pthread_setname_np(t->tid, name)) {
            loge("pthread_setname_np failed(%d): %s\n", errno, strerror(errno));
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
