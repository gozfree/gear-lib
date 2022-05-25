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

#include <libposix.h>
#include <libdarray.h>
#include <libthread.h>

#define LIBWORKQ_VERSION "0.1.2"

#ifdef __cplusplus
extern "C" {
#endif

struct workq {
    int run;
    int load;
    struct thread *thread;
    struct list_head wq_list;
};

typedef struct workq_pool {
    int cpus;
    int threshold;
    DARRAY(struct workq *) wq_array;
} workq_pool_t;

typedef void (*task_func_t)(void *);

struct task {
    struct list_head entry;
    task_func_t func;
    void *data;
    struct workq *wq;
};

GEAR_API struct workq_pool *workq_pool_create();
GEAR_API int workq_pool_task_push(struct workq_pool *p, task_func_t f, void *d);
GEAR_API void workq_pool_destroy(struct workq_pool *pool);

#ifdef __cplusplus
}
#endif
#endif
