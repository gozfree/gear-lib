/******************************************************************************
 * Copyright (C) 2014-2020 Zhifeng Gong <gozfree@163.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 ******************************************************************************/
#ifndef LIBWORKQ_H
#define LIBWORKQ_H

#include <gear-lib/libmacro.h>
#include <gear-lib/libgevent.h>
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
