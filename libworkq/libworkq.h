/*****************************************************************************
 * Copyright (C) 2014-2015
 * file:    libworkq.h
 * author:  gozfree <gozfree@163.com>
 * created: 2015-07-13 01:51
 * updated: 2015-07-19 20:44
 *****************************************************************************/
#ifndef LIBWORKQ_H
#define LIBWORKQ_H

#include <pthread.h>
#include <libgevent.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct workq {
    int loop;
    pthread_t tid;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    struct list_head wq_list;
    struct gevent_base *evbase;
} workqueue_t;

typedef struct workq_pool {
    int cpus;
    int ring;
    struct workq *wq;//multi workq
} workq_pool_t;

typedef struct worker {
    struct list_head entry;
    void (*func)(void *);
    void *data;
    struct workq *wq;//root
} worker_t;

typedef void (*worker_func_t)(void *);

struct workq *wq_create();
void wq_destroy(struct workq *wq);
void wq_task_add(struct workq *wq, worker_func_t func, void *data, size_t len);

/* high level */
int wq_pool_init();
int wq_pool_task_add(worker_func_t func, void *data, size_t len);
void wq_pool_deinit();


#ifdef __cplusplus
}
#endif
#endif
