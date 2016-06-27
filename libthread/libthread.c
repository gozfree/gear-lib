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
    printf("thread %s is created, thread_id = %ld, thread_self = %ld\n",
             t->name, t->tid, pthread_self());
    t->func(t, t->arg);
    printf("thread %s exits\n", t->name);

    return NULL;
}

static void *__thread_func_std(void *arg)
{
#if 0
    struct thread *t = (struct thread *)arg;
    if (!t->func) {
        printf("thread function is null\n");
        return NULL;
    }
    printf("thread %s is created, thread_id = %ld, thread_self = %ld\n",
           t->name, t->tid, pthread_self());
    void *p = t->arg;
    va_list list;
    va_start(ap, t->fmt);
    va_copy(list, ap);
    va_end(ap);
    
    t->func_std(t, t->fmt, t->arg);
    printf("thread %s exits\n", t->name);
#endif

    return NULL;
}


struct thread *thread_create(void *(*func)(struct thread *, void *), void *arg)
{
    int ret;
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
    if (t->spin) spin_deinit(t->spin);
    if (t->sem) sem_lock_deinit(t->sem);
    if (t->mutex) mutex_lock_deinit(t->mutex);
    if (t->cond) mutex_cond_deinit(t->cond);
    if (t) {
        free(t);
    }
    return NULL;
}

struct thread *thread_create_std(const char *name,
                void *(*func)(struct thread *, const char *fmt, ...),
                const char *fmt, ...)
{
    int ret;
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

    char *p = NULL;
    int size = 0;
    va_start(ap, fmt);
    size = vsnprintf(p, size, fmt, ap);
    va_end(ap);
    if (size < 0) {
        printf("get args failed:%s\n", strerror(errno));
        goto err;
    }

    size++;/* For '\0' */
    p = calloc(1, size);
    if (!p) {
        goto err;
    }

    va_start(ap, fmt);
    size = vsnprintf(p, size, fmt, ap);
    va_end(ap);
    if (size < 0) {
        printf("get args failed:%s\n", strerror(errno));
        goto err;
    }

    t->arg = (void *)p;
    t->fmt = strdup(fmt);
    t->func_std = func;

    if (0 != pthread_create(&t->tid, NULL, __thread_func_std, t)) {
        printf("pthread_create failed(%d): %s\n", errno, strerror(errno));
        goto err;
    }
    if (name && strlen(name) > 0) {
        if (strlen(name) > PTHREAD_NAME_LEN) {
            printf("thread name is out of range, should be less than %d\n",
                   PTHREAD_NAME_LEN);
        }
        strncpy(t->name, name, PTHREAD_NAME_LEN);
        if (0 != (ret = pthread_setname_np(t->tid, t->name))) {
            if (ret == ERANGE) {
                printf("thread name is out of range, should be less than %d\n",
                                PTHREAD_NAME_LEN);
            } else {
                printf("pthread_setname_np ret = %d, failed(%d): %s\n",
                                ret, errno, strerror(errno));
            }
            goto err;
        }
    }

    return t;

err:
    if (t->spin) spin_deinit(t->spin);
    if (t->sem) sem_lock_deinit(t->sem);
    if (t->mutex) mutex_lock_deinit(t->mutex);
    if (t->cond) mutex_cond_deinit(t->cond);
    if (t->arg) {
        free(t->arg);
    }
    if (t->fmt) {
        free(t->fmt);
    }
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
    if (t->spin) spin_deinit(t->spin);
    if (t->sem) sem_lock_deinit(t->sem);
    if (t->mutex) mutex_lock_deinit(t->mutex);
    if (t->cond) mutex_cond_deinit(t->cond);
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
    return mutex_cond_signal(t->cond);
}
int thread_cond_signal_all(struct thread *t)
{
    if (!t || !t->cond) {
        return -1;
    }
    return mutex_cond_signal_all(t->cond);
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

void thread_print_info(struct thread *t)
{
    pthread_mutex_lock(&t->mutex);
    printf("========\n");
    printf("thread name = %s\n", t->name);
    printf("thread id = %ld\n", t->tid);
    printf("========\n");
    pthread_mutex_unlock(&t->mutex);
}

