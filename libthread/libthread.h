/*****************************************************************************
 * Copyright (C) 2014-2015
 * file:    libthread.h
 * author:  gozfree <gozfree@163.com>
 * created: 2015-08-15 22:57
 * updated: 2015-08-15 22:57
 *****************************************************************************/
#ifndef _LIBTHREAD_H_
#define _LIBTHREAD_H_

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

struct thread {
    pthread_t tid;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    void *(*func)(struct thread *, void *);
    void *arg;
} thread_t;

struct thread *thread_create(void *(*func)(struct thread *, void *), void *arg);
void thread_destroy(struct thread *t);
int thread_cond_wait(struct thread *t);
int thread_cond_signal(struct thread *t);
int thread_mutex_lock(struct thread *t);
int thread_mutex_unlock(struct thread *t);


#ifdef __cpulspuls
}
#endif
#endif
