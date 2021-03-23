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
#include "libworkq.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#if defined (OS_LINUX)
#include <sys/sysinfo.h>
#endif

static void worker_destroy(struct worker *w);
static struct workq_pool _wpool;

static void *_worker_thread(struct thread *t, void *arg)
{
    struct workq *wq = (struct workq *)arg;
    struct worker *w;
    while (wq->loop) {
        thread_lock(t);
        while (list_empty(&wq->wq_list) && wq->loop) {
            thread_wait(t, 0);
        }
        w = list_first_entry_or_null(&wq->wq_list, struct worker, entry);
        thread_unlock(t);
        if (w && w->func) {
            w->func(w->data);
            worker_destroy(w);
        }
    }
    return NULL;
}

struct worker *first_worker(struct workq *wq)
{
    if (list_empty(&wq->wq_list))
        return NULL;

    return list_first_entry_or_null(&wq->wq_list, struct worker, entry);
}

static int _wq_init(struct workq *wq)
{
    INIT_LIST_HEAD(&wq->wq_list);
    wq->loop = 1;
    wq->thread = thread_create(_worker_thread, wq);
    return 0;
}

static void _wq_deinit(struct workq *wq)
{
    struct worker *w;
    thread_lock(wq->thread);
    while ((w = first_worker(wq))) {
        thread_unlock(wq->thread);
        worker_destroy(w);
        thread_lock(wq->thread);
    }
    wq->loop = 0;
    thread_signal(wq->thread);
    thread_unlock(wq->thread);
    thread_join(wq->thread);
    thread_destroy(wq->thread);
}

static void worker_create(struct workq *wq, worker_func_t func, void *data, size_t len)
{
    struct worker *w = CALLOC(1, struct worker);
    if (!w) {
        return;
    }
    w->wq = wq;
    w->func = func;
    w->data = memdup(data, len);
    thread_lock(wq->thread);
    list_add_tail(&w->entry, &wq->wq_list);
    thread_signal(wq->thread);
    thread_unlock(wq->thread);
}

static void worker_destroy(struct worker *w)
{
    struct workq *wq;
    wq = w->wq;
    thread_lock(wq->thread);
    list_del_init(&w->entry);
    thread_unlock(wq->thread);
    free(w->data);
    free(w);
}

/* wq_xxx API */
struct workq *wq_create()
{
    struct workq *wq = CALLOC(1, struct workq);
    if (!wq) {
        return NULL;
    }
    if (0 != _wq_init(wq)) {
        free(wq);
        return NULL;
    }
    return wq;
}

void wq_destroy(struct workq *wq)
{
    _wq_deinit(wq);
    free(wq);
}

void wq_task_add(struct workq *wq, worker_func_t func, void *data, size_t len)
{
    worker_create(wq, func, data, len);
}

/* wq_pool_xxx API */
int wq_pool_init()
{
    int i;
#ifdef OS_ANDROID
    int cpus = 1;
#elif defined (OS_LINUX)
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
    printf("create %d cpus thread\n", cpus);
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
    worker_create(wq, func, data, len);
    return 0;
}
