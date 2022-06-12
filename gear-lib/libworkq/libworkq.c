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

struct task {
    struct list_head entry;
    task_func_t func;
    void *data;
    struct workq *wq;
};


static bool is_workq_underload(struct workq_pool *pool, struct workq *wq)
{
    return (wq->load == 0 || wq->load < pool->threshold);
}

bool is_workq_overload(struct workq_pool *pool, struct workq *wq)
{
    return (wq->load == 1 || wq->load > pool->threshold);
}

static struct task *task_create(struct workq *wq, task_func_t func, void *data)
{
    struct task *t = calloc(1, sizeof(struct task));
    if (!t) {
        return NULL;
    }
    t->wq = wq;
    t->func = func;
    t->data = data;
    thread_lock(wq->thread);
    list_add_tail(&t->entry, &wq->wq_list);
    thread_signal(wq->thread);
    thread_unlock(wq->thread);
    return t;
}

static void task_destroy(struct task *t)
{
    struct workq *wq = t->wq;
    thread_lock(wq->thread);
    list_del_init(&t->entry);
    thread_unlock(wq->thread);
    free(t);
}

static void *_task_thread(struct thread *thread, void *arg)
{
    struct workq *wq = (struct workq *)arg;
    struct task *t;
    while (wq->run) {
        thread_lock(thread);
        while (list_empty(&wq->wq_list) && wq->run) {
            thread_wait(thread, 0);
        }
        t = list_first_entry_or_null(&wq->wq_list, struct task, entry);
        thread_unlock(thread);
        if (t && t->func) {
            wq->load = 1;
            t->func(t->data);
            task_destroy(t);
            wq->load = 0;
        }
    }
    return NULL;
}

static struct workq *workq_create()
{
    struct workq *wq = calloc(1, sizeof(struct workq));
    if (!wq) {
        return NULL;
    }
    INIT_LIST_HEAD(&wq->wq_list);
    wq->run = 1;
    wq->load = 0;
    wq->thread = thread_create(_task_thread, wq);
    if (!wq->thread) {
        printf("thread create failed!\n");
        free(wq);
        return NULL;
    }
    return wq;
}

static void workq_destroy(struct workq *wq)
{
    struct task *t;

    thread_lock(wq->thread);
    while (!list_empty(&wq->wq_list)) {
        t = list_first_entry_or_null(&wq->wq_list, struct task, entry);
        task_destroy(t);
    }
    wq->run = 0;
    thread_signal(wq->thread);
    thread_unlock(wq->thread);

    thread_join(wq->thread);
    thread_destroy(wq->thread);
}

struct workq_pool *workq_pool_create()
{
    int i;
    int cpus = 1;
    struct workq *wq;

    struct workq_pool *pool = calloc(1, sizeof(struct workq_pool));
    if (!pool) {
        printf("malloc workq_pool failed!\n");
        return NULL;
    }

#if defined (OS_LINUX)
    cpus = get_nprocs_conf();
#endif
    if (cpus <= 0) {
        printf("cpu number is invalid!\n");
        goto failed;
    }
    printf("cpu number is %d\n", cpus);

    pool->cpus = cpus;
    pool->threshold = 0;
    da_init(pool->wq_array);

    for (i = 0; i < cpus; ++i) {
        wq = workq_create();
        if (!wq) {
            goto failed;
        }
        da_push_back(pool->wq_array, &wq);
    }

    return pool;

failed:
    if (pool) {
        do {
            wq = pool->wq_array.array[pool->wq_array.num-1];
            da_erase_item(pool->wq_array, &wq);
            free(wq);
        } while (pool->wq_array.num > 0);
    }
    if (!pool) {
        free(pool);
    }
    return NULL;
}

static struct workq *find_underrun_workq(struct workq_pool *pool)
{
    int i;
    struct workq *wq;
    for (i = 0; i < pool->wq_array.num; i++) {
        wq = pool->wq_array.array[i];
        if (is_workq_underload(pool, wq))
            break;
    }
    if (i == pool->wq_array.num) {
        /*
         * TODO: need do load blance
         */
        i = pool->wq_array.num - 1;
        printf("all workq are overload, need expand, just force to last workq!\n");
    }
    return pool->wq_array.array[i];
}

int workq_pool_task_push(struct workq_pool *pool, task_func_t func, void *data)
{
    struct task *t;
    struct workq *wq;
    if (!pool || !func) {
        printf("invalid paraments!\n");
        return -1;
    }
    wq = find_underrun_workq(pool);
    if (!wq) {
        printf("all workq are not idle, need expand\n");
        return -1;
    }
    t = task_create(wq, func, data);
    if (!t) {
        return -1;
    }

    return 0;
}

void workq_pool_destroy(struct workq_pool *pool)
{
    struct workq *wq;
    if (!pool) {
        return;
    }

    while (pool->wq_array.num > 0) {
        wq = pool->wq_array.array[pool->wq_array.num-1];
        workq_destroy(wq);
        da_erase_item(pool->wq_array, &wq);
        free(wq);
    }
    da_free(pool->wq_array);
    free(pool);
}
