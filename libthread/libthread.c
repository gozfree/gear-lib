/*****************************************************************************
 * Copyright (C) 2014-2015
 * file:    libthread.c
 * author:  gozfree <gozfree@163.com>
 * created: 2015-08-15 22:57
 * updated: 2016-01-03 15:38
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include "libthread.h"

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

struct thread *thread_create(void *(*func)(struct thread *, void *), void *arg)
{
    struct thread *t = CALLOC(1, struct thread);
    if (!t) {
        printf("malloc thread failed(%d): %s\n", errno, strerror(errno));
        goto err;
    }
    if (!(t->spin = spin_lock_init())) {
        printf("spin_lock_init failed\n");
        goto err;
    }
    if (!(t->mutex = mutex_lock_init())) {
        printf("mutex_lock_init failed\n");
        goto err;
    }
    if (!(t->cond = mutex_cond_init())) {
        printf("mutex_cond_init failed\n");
        goto err;
    }
    if (!(t->sem = sem_lock_init())) {
        printf("sem_lock_init failed\n");
        goto err;
    }

    t->arg = arg;
    t->func = func;
    if (0 != pthread_create(&t->tid, NULL, __thread_func, t)) {
        printf("pthread_create failed(%d): %s\n", errno, strerror(errno));
        goto err;
    }
    return t;

err:
    if (t->spin) spin_lock_deinit(t->spin);
    if (t->sem) sem_lock_deinit(t->sem);
    if (t->mutex) mutex_lock_deinit(t->mutex);
    if (t->cond) mutex_cond_deinit(t->cond);
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
    if (t->spin) spin_lock_deinit(t->spin);
    if (t->sem) sem_lock_deinit(t->sem);
    if (t->mutex) mutex_lock_deinit(t->mutex);
    if (t->cond) mutex_cond_deinit(t->cond);
    pthread_join(t->tid, NULL);
    free(t);
}

int thread_spin_lock(struct thread *t)
{
    if (!t || !t->spin) {
        return -1;
    }
    return spin_lock(t->spin);
}
int thread_spin_unlock(struct thread *t)
{
    if (!t || !t->spin) {
        return -1;
    }
    return spin_unlock(t->spin);
}

int thread_mutex_lock(struct thread *t)
{
    if (!t || !t->mutex) {
        return -1;
    }
    return mutex_lock(t->mutex);
}
int thread_mutex_unlock(struct thread *t)
{
    if (!t || !t->mutex) {
        return -1;
    }
    return mutex_unlock(t->mutex);
}

int thread_cond_wait(struct thread *t, int64_t ms)
{
    if (!t || !t->mutex || !t->cond) {
        return -1;
    }
    return mutex_cond_wait(t->mutex, t->cond, ms);
}
int thread_cond_signal(struct thread *t)
{
    if (!t || !t->cond) {
        return -1;
    }
    mutex_cond_signal(t->cond);
    return 0;
}
int thread_cond_signal_all(struct thread *t)
{
    if (!t || !t->cond) {
        return -1;
    }
    mutex_cond_signal_all(t->cond);
    return 0;
}


int thread_sem_wait(struct thread *t, int64_t ms)
{
    if (!t || !t->sem) {
        return -1;
    }
    return sem_lock_wait(t->sem, ms);
}
int thread_sem_signal(struct thread *t)
{
    if (!t || !t->sem) {
        return -1;
    }
    return sem_lock_signal(t->sem);
}


