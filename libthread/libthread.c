/*****************************************************************************
 * Copyright (C) 2014-2015
 * file:    libthread.c
 * author:  gozfree <gozfree@163.com>
 * created: 2015-08-15 22:57
 * updated: 2015-08-15 22:57
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <libgzf.h>
#include "libthread.h"


static void *__thread_func(void *arg)
{
    struct thread *t = (struct thread *)arg;
    if (t->func) {
        printf("%s:%d t->tid = %ld\n", __func__, __LINE__, t->tid);
        t->func(t, t->arg);
    }

    return NULL;
}

struct thread *thread_create(void *(*func)(struct thread *, void *), void *arg)
{
    struct thread *t = CALLOC(1, struct thread);
    if (!t) {
        printf("malloc thread failed(%d): %s\n", errno, strerror(errno));
        goto err;
    }
    pthread_mutex_init(&t->mutex, NULL);
    if (0 != pthread_cond_init(&t->cond, NULL)) {
        printf("pthread_cond_init failed(%d): %s\n", errno, strerror(errno));
        goto err;
    }
    t->arg = arg;
    t->func = func;
    if (0 != pthread_create(&t->tid, NULL, __thread_func, t)) {
        printf("pthread_create failed(%d): %s\n", errno, strerror(errno));
        goto err;
    }
    printf("%s:%d t->tid = %ld\n", __func__, __LINE__, t->tid);
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
