/*****************************************************************************
 * Copyright (C) 2014-2015
 * file:    libworkq.c
 * author:  gozfree <gozfree@163.com>
 * created: 2015-07-13 01:51
 * updated: 2015-07-19 20:44
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <signal.h>
#include <sys/sysinfo.h>
#include <libmacro.h>
#include "libworkq.h"

static void wq_worker_destroy(struct worker *w);
static struct workq_pool _wpool;

static void *wq_thread(void *arg)
{
    struct workq *wq = (struct workq *)arg;
    struct worker *w, *n;
    while (wq->loop) {
        pthread_mutex_lock(&wq->mutex);
        list_for_each_entry_safe(w, n, &wq->wq_list, entry) {
            pthread_mutex_unlock(&wq->mutex);
            if (w->func) {
                w->func(w->data);
            }
            wq_worker_destroy(w);
            pthread_mutex_lock(&wq->mutex);
        }
        pthread_mutex_unlock(&wq->mutex);
        gevent_base_wait(wq->evbase);
    }

    return NULL;
}

static struct worker *first_worker(struct workq *wq)
{
    if (list_empty(&wq->wq_list))
        return NULL;

    return list_first_entry(&wq->wq_list, struct worker, entry);
}


static int _wq_init(struct workq *wq)
{
    if (!wq) {
        return -1;
    }
    INIT_LIST_HEAD(&wq->wq_list);
    wq->loop = 1;
    wq->evbase = gevent_base_create();
    pthread_mutex_init(&wq->mutex, NULL);
    pthread_cond_init(&wq->cond, NULL);
    pthread_create(&wq->tid, NULL, wq_thread, wq);
    return 0;
}

struct workq *wq_create()
{
    struct workq *wq = CALLOC(1, struct workq);
    if (0 != _wq_init(wq)) {
        return NULL;
    }
    return wq;
}

static void _wq_deinit(struct workq *wq)
{
    struct worker *w;
    if (!wq) {
        return;
    }
    pthread_mutex_lock(&wq->mutex);
    while ((w = first_worker(wq))) {
        pthread_mutex_unlock(&wq->mutex);
        wq_worker_destroy(w);
        pthread_mutex_lock(&wq->mutex);
    }
    pthread_mutex_unlock(&wq->mutex);
    wq->loop = 0;
    gevent_base_signal(wq->evbase);
    pthread_join(wq->tid, NULL);
    gevent_base_destroy(wq->evbase);
    pthread_cond_destroy(&wq->cond);
    pthread_mutex_destroy(&wq->mutex);
}

void wq_destroy(struct workq *wq)
{
    _wq_deinit(wq);
    free(wq);
}

static void wq_worker_create(struct workq *wq, worker_func_t func, void *data, size_t len)
{
    struct worker *w = CALLOC(1, struct worker);
    if (!w) {
        return;
    }
    w->wq = wq;
    w->func = func;
    w->data = calloc(1, len);
    memcpy(w->data, data, len);
    pthread_mutex_lock(&wq->mutex);
    list_add_tail(&w->entry, &wq->wq_list);
    pthread_mutex_unlock(&wq->mutex);
    gevent_base_signal(wq->evbase);
}

void wq_task_add(struct workq *wq, worker_func_t func, void *data, size_t len)
{
    wq_worker_create(wq, func, data, len);
}

static void wq_worker_destroy(struct worker *w)
{
    struct workq *wq;
    if (!w) {
        return;
    }
    wq = w->wq;
    pthread_mutex_lock(&wq->mutex);
    list_del_init(&w->entry);
    pthread_mutex_unlock(&wq->mutex);
    free(w->data);
    free(w);
}

int wq_pool_init()
{
    int i;
#ifdef __ANDROID__
    int cpus = 1;
#else
    int cpus = get_nprocs_conf();
#endif
    if (cpus <= 0) {
        printf("get_nprocs_conf failed!\n");
        return -1;
    }
    struct workq *wq = CALLOC(cpus, struct workq);
    if (!wq) {
        printf("malloc workq failed!\n");
        return -1;
    }
    _wpool.wq = wq;
    for (i = 0; i < cpus; ++i, ++wq) {
        if (0 != _wq_init(wq)) {
            printf("_wq_init failed!\n");
            return -1;
        }
    }
    _wpool.cpus = cpus;
    _wpool.ring = 0;
    return 0;
}

void wq_pool_deinit()
{
    int i;
    struct workq *wq = _wpool.wq;
    if (!wq) {
        return;
    }
    for (i = 0; i < _wpool.cpus; ++i, ++wq) {
        _wq_deinit(wq);
    }
    free(_wpool.wq);
}

int wq_pool_task_add(worker_func_t func, void *data, size_t len)
{
    if (!func) {
        printf("invalid func ptr!\n");
        return -1;
    }
    int i = ++_wpool.ring % _wpool.cpus;
    struct workq *wq = _wpool.wq+i;
    wq_worker_create(wq, func, data, len);
    return 0;
}
