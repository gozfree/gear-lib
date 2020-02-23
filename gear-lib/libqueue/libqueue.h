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

#if defined (__linux__) || defined (__CYGWIN__)
#include <sys/uio.h>
#include <pthread.h>
#elif defined (__WIN32__) || defined (WIN32) || defined (_MSC_VER)
#include "libposix4win.h"
#endif
#include <gear-lib/libmacro.h>

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


struct item {
    struct iovec data;
    struct list_head entry;
    struct iovec opaque;
    int ref_cnt;
};

struct queue;

typedef void *(alloc_hook)(void *data, size_t len, void *arg);
typedef void (free_hook)(void *data);

struct queue_branch {
    char *name;
#define RD_FD   fds[0]
#define WR_FD   fds[1]
    int fds[2]; //use pipe fds to trigger and poll queue, rfd[0] wfd[1]
    struct list_head hook;
};

struct queue {
    struct list_head head;
    int depth;
    int max_depth;
    pthread_mutex_t lock;
    pthread_cond_t cond;
    enum queue_mode mode;
    alloc_hook *alloc_hook;
    free_hook *free_hook;
    struct list_head branch;
    int branch_cnt;
    struct iovec opaque;
};

struct item *item_alloc(struct queue *q, void *data, size_t len, void *arg);
void item_free(struct queue *q, struct item *item);
struct iovec *item_get_data(struct queue *q, struct item *it);

struct queue *queue_create();
void queue_destroy(struct queue *q);
int queue_set_depth(struct queue *q, int depth);
int queue_get_depth(struct queue *q);
int queue_set_mode(struct queue *q, enum queue_mode mode);
int queue_set_hook(struct queue *q, alloc_hook *alloc_cb, free_hook *free_cb);
struct item *queue_pop(struct queue *q);
int queue_push(struct queue *q, struct item *item);
int queue_flush(struct queue *q);

struct queue_branch *queue_branch_new(struct queue *q, const char *name);
int queue_branch_del(struct queue *q, const char *name);
int queue_branch_notify(struct queue *q);
struct item *queue_branch_pop(struct queue *q, const char *name);
struct queue_branch *queue_branch_get(struct queue *q, const char *name);

#ifdef __cplusplus
}
#endif

#endif
