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
#ifndef LIBQUEUE_H
#define LIBQUEUE_H

#include <libposix.h>
#include <pthread.h>

#define LIBQUEUE_VERSION "0.2.2"

/*
 * queue is multi-reader single-writer
 *
 *                 |-->branch1
 * t1-->t2-->...-->tN
 *                 |-->branch2
 */


#ifdef __cplusplus
extern "C" {
#endif

enum queue_mode {
    QUEUE_FULL_FLUSH = 0,
    QUEUE_FULL_RING,
};


struct queue_item {
    struct list_head entry;
    struct iovec     data;
    struct iovec     opaque;
    void            *arg;
    int              ref_cnt;
};

struct queue;

typedef void *(queue_alloc_hook)(void *data, size_t len, void *arg);
typedef void (queue_free_hook)(void *data);

struct queue_branch {
    char             *name;
    int              evfd;
    struct list_head hook;
};

struct queue {
    struct list_head  head;
    int               depth;
    int               max_depth;
    pthread_mutex_t   lock;
    pthread_cond_t    cond;
    enum queue_mode   mode;
    queue_alloc_hook *alloc_hook;
    queue_free_hook  *free_hook;
    struct list_head  branch;
    int               branch_cnt;
    struct iovec      opaque;
};

GEAR_API struct queue_item *queue_item_alloc(struct queue *q, void *data, size_t len, void *arg);
GEAR_API void queue_item_free(struct queue *q, struct queue_item *item);
GEAR_API struct iovec *queue_item_get_data(struct queue *q, struct queue_item *it);

GEAR_API struct queue *queue_create();
GEAR_API void queue_destroy(struct queue *q);
GEAR_API int queue_set_depth(struct queue *q, int depth);
GEAR_API int queue_get_depth(struct queue *q);
GEAR_API int queue_set_mode(struct queue *q, enum queue_mode mode);
GEAR_API int queue_set_hook(struct queue *q, queue_alloc_hook *alloc_cb, queue_free_hook *free_cb);
GEAR_API struct queue_item *queue_pop(struct queue *q);
GEAR_API int queue_push(struct queue *q, struct queue_item *item);
GEAR_API int queue_flush(struct queue *q);

GEAR_API struct queue_branch *queue_branch_new(struct queue *q, const char *name);
GEAR_API int queue_branch_del(struct queue *q, const char *name);
GEAR_API int queue_branch_notify(struct queue *q);
GEAR_API struct queue_item *queue_branch_pop(struct queue *q, const char *name);
GEAR_API struct queue_branch *queue_branch_get(struct queue *q, const char *name);

#ifdef __cplusplus
}
#endif

#endif
