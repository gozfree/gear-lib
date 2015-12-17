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
#include <semaphore.h>
#include <libgzf.h>

#ifdef __cplusplus
extern "C" {
#endif

struct thread {
    pthread_t tid;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    sem_t sem;
    char *name;  //only for debug, useless
    int is_run;
    void *(*func)(struct thread *, void *);
    void *arg;
} thread_t;

int thread_capability(struct capability_desc *desc);
struct thread *thread_create(const char *name,
                void *(*func)(struct thread *, void *), void *arg);
void thread_destroy(struct thread *t);
int thread_cond_wait(struct thread *t);
int thread_cond_signal(struct thread *t);
int thread_mutex_lock(struct thread *t);
int thread_mutex_unlock(struct thread *t);
int thread_sem_lock(struct thread *t);
int thread_sem_unloock(struct thread *t);
int thread_sem_wait(struct thread *t, int64_t ms);
int thread_sem_signal(struct thread *t);
void thread_print_info(struct thread *t);


/*****************************************************************************
 below APIs defined for dynamic called
 *****************************************************************************/
typedef int (*dl_thread_capability)(struct capability_desc *desc);

typedef struct thread *(*dl_thread_create)(const char *name,
                void *(*func)(struct thread *, void *), void *arg);
typedef void (*dl_thread_destroy)(struct thread *t);
typedef int (*dl_thread_cond_wait)(struct thread *t);
typedef int (*dl_thread_cond_signal)(struct thread *t);
typedef int (*dl_thread_mutex_lock)(struct thread *t);
typedef int (*dl_thread_mutex_unlock)(struct thread *t);
typedef int (*dl_thread_sem_lock)(struct thread *t);
typedef int (*dl_thread_sem_unloock)(struct thread *t);
typedef int (*dl_thread_sem_wait)(struct thread *t, int64_t ms);
typedef int (*dl_thread_sem_signal)(struct thread *t);
typedef void (*dl_thread_print_info)(struct thread *t);

enum thread_cap {
    THREAD_CAPABILITY = 0,
    THREAD_CREATE,
    THREAD_DESTROY,
    THREAD_COND_WAIT,
    THREAD_COND_SIGNAL,
    THREAD_MUTEX_LOCK,
    THREAD_MUTEX_UNLOCK,
    THREAD_SEM_LOCK,
    THREAD_SEM_UNLOCK,
    THREAD_SEM_WAIT,
    THREAD_SEM_SIGNAL,
    THREAD_PRINT_INFO,
};

#ifdef __cpulspuls
}
#endif
#endif
