/******************************************************************************
 * Copyright (C) 2014-2018 Zhifeng Gong <gozfree@163.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with libraries; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 ******************************************************************************/
#ifndef LIBWORKQ_H
#define LIBWORKQ_H

#include <libmacro.h>
#include <libgevent.h>
#include <pthread.h>

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
