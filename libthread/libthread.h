/*****************************************************************************
 * Copyright (C) 2014-2015
 * file:    libthread.h
 * author:  gozfree <gozfree@163.com>
 * created: 2015-08-15 22:57
 * updated: 2016-01-03 15:38
 *****************************************************************************/
#ifndef LIBTHREAD_H
#define LIBTHREAD_H

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <liblock.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PTHREAD_NAME_LEN    16
typedef struct thread {
    pthread_t tid;
    spin_lock_t *spin;
    mutex_lock_t *mutex;
    mutex_cond_t *cond;
    sem_lock_t *sem;
    char name[PTHREAD_NAME_LEN];
    void *(*func)(struct thread *, void *);
    char *fmt;
    void *arg;
} thread_t;

struct thread *thread_create(void *(*func)(struct thread *, void *), void *arg);

void thread_destroy(struct thread *t);

int thread_spin_lock(struct thread *t);
int thread_spin_unlock(struct thread *t);

int thread_sem_wait(struct thread *t, int64_t ms);
int thread_sem_signal(struct thread *t);

int thread_mutex_lock(struct thread *t);
int thread_mutex_unlock(struct thread *t);
int thread_cond_wait(struct thread *t, int64_t ms);
int thread_cond_signal(struct thread *t);
int thread_cond_signal_all(struct thread *t);


#ifdef __cplusplus
}
#endif
#endif
