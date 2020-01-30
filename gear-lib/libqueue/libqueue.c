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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#if defined (__linux__) || defined (__CYGWIN__)
#include <unistd.h>
#include <sys/time.h>
#endif
#include "libqueue.h"

#define QUEUE_MAX_DEPTH 200

struct iovec *item_get_data(struct queue *q, struct item *it)
{
    if (q->alloc_hook) {
        return &it->opaque;
    } else {
        return &it->data;
    }
}

struct item *item_alloc(struct queue *q, void *data, size_t len, void *arg)
{
    struct item *item;
    if (!q) {
        return NULL;
    }
    item = CALLOC(1, struct item);
    if (!item) {
        printf("malloc failed!\n");
        return NULL;
    }
    if (q->alloc_hook) {
        item->opaque.iov_base = (q->alloc_hook)(data, len, arg);
        item->opaque.iov_len = len;
    } else {
        item->data.iov_base = memdup(data, len);
        item->data.iov_len = len;
    }
    item->ref_cnt = q->branch_cnt;
    return item;
}

void item_free(struct queue *q, struct item *item)
{
    if (!q) {
        return;
    }
    if (!item) {
        return;
    }
    if (q->free_hook) {
        (q->free_hook)(item->opaque.iov_base);
        item->opaque.iov_len = 0;
    } else {
        free(item->data.iov_base);
    }
    free(item);
}

int queue_set_mode(struct queue *q, enum queue_mode mode)
{
    if (!q) {
        return -1;
    }
    q->mode = mode;
    return 0;
}

int queue_set_hook(struct queue *q, alloc_hook *alloc_cb, free_hook *free_cb)
{
    if (!q) {
        return -1;
    }
    q->alloc_hook = alloc_cb;
    q->free_hook = free_cb;
    return 0;
}

int queue_set_depth(struct queue *q, int depth)
{
    if (!q) {
        return -1;
    }
    q->max_depth = depth;
    return 0;
}

int queue_get_depth(struct queue *q)
{
    return q->depth;
}

struct queue *queue_create()
{
    struct queue *q = CALLOC(1, struct queue);
    if (!q) {
        printf("malloc failed!\n");
        return NULL;
    }
    INIT_LIST_HEAD(&q->head);
    INIT_LIST_HEAD(&q->branch);
    pthread_mutex_init(&q->lock, NULL);
    pthread_cond_init(&q->cond, NULL);
    q->depth = 0;
    q->max_depth = QUEUE_MAX_DEPTH;
    q->mode = QUEUE_FULL_FLUSH;
    q->alloc_hook = NULL;
    q->free_hook = NULL;
    q->branch_cnt = 0;
    return q;
}

int queue_flush(struct queue *q)
{
    struct item *item, *next;
    if (!q) {
        return -1;
    }
    pthread_mutex_lock(&q->lock);
#if defined (__linux__) || defined (__CYGWIN__)
    list_for_each_entry_safe(item, next, &q->head, entry) {
#elif defined (__WIN32__) || defined (WIN32) || defined (_MSC_VER)
    list_for_each_entry_safe(item, struct item, next, struct item, &q->head, entry) {
#endif
        list_del(&item->entry);
        item_free(q, item);
    }
    q->depth = 0;
    pthread_cond_signal(&q->cond);
    pthread_mutex_unlock(&q->lock);
    return 0;
}

void queue_destroy(struct queue *q)
{
    if (!q) {
        return;
    }
    queue_flush(q);
    pthread_mutex_destroy(&q->lock);
    pthread_cond_destroy(&q->cond);
    free(q);
}

void queue_pop_free(struct queue *q)
{
    struct item *tmp = queue_pop(q);
    if (tmp) {
        item_free(q, tmp);
    }
}

int queue_push(struct queue *q, struct item *item)
{
    if (!q || !item) {
        printf("invalid paraments!\n");
        return -1;
    }
    if (q->depth >= q->max_depth) {
        if (q->mode == QUEUE_FULL_FLUSH) {
            queue_flush(q);
        } else if (q->mode == QUEUE_FULL_RING) {
            queue_pop_free(q);
        }
    }
    pthread_mutex_lock(&q->lock);
    list_add_tail(&item->entry, &q->head);
    ++(q->depth);
    queue_branch_notify(q);
    pthread_cond_signal(&q->cond);
    pthread_mutex_unlock(&q->lock);
    if (q->depth > q->max_depth) {
        printf("queue depth reach max depth %d\n", q->depth);
    }
    return 0;
}

struct item *queue_pop(struct queue *q)
{
    struct item *item = NULL;
    if (!q) {
        printf("invalid parament!\n");
        return NULL;
    }

    pthread_mutex_lock(&q->lock);
    while (list_empty(&q->head)) {
#if defined (__linux__) || defined (__CYGWIN__)
        struct timeval now;
        struct timespec outtime;
        gettimeofday(&now, NULL);
        outtime.tv_sec = now.tv_sec + 1;
        outtime.tv_nsec = now.tv_usec * 1000;
        int ret = pthread_cond_timedwait(&q->cond, &q->lock, &outtime);
#elif defined (__WIN32__) || defined (WIN32) || defined (_MSC_VER)
        int ret = pthread_cond_timedwait(&q->cond, &q->lock, 1000);
#endif
        if (ret == 0) {
            break;
        }
        switch (ret) {
        case ETIMEDOUT:
            //printf("the condition variable was not signaled "
            //       "until the timeout specified by abstime.\n");
            break;
        case EINTR:
            printf("pthread_cond_timedwait was interrupted by a signal.\n");
            break;
        default:
            printf("pthread_cond_timedwait error:%s.\n", strerror(ret));
            break;
        }
    }

    item = list_first_entry_or_null(&q->head, struct item, entry);
    if (item) {
        --item->ref_cnt;
        if (item->ref_cnt <= 0) {
            list_del(&item->entry);
            --(q->depth);
        }
    }
    pthread_mutex_unlock(&q->lock);
    return item;
}

struct queue_branch *queue_branch_new(struct queue *q, const char *name)
{
    struct queue_branch *qb = CALLOC(1, struct queue_branch);
    if (!q || !qb || !name) {
        return NULL;
    }
    if (pipe(qb->fds)) {
        printf("create pipe failed: %s\n", strerror(errno));
        return NULL;
    }
    qb->name = strdup(name);
    list_add_tail(&qb->hook, &q->branch);
    q->branch_cnt++;
    return qb;
}

int queue_branch_del(struct queue *q, const char *name)
{
    struct queue_branch *qb, *next;
#if defined (__linux__) || defined (__CYGWIN__)
    list_for_each_entry_safe(qb, next, &q->branch, hook) {
#else
    list_for_each_entry_safe(qb, struct queue_branch, next, struct queue_branch, &q->branch, hook) {
#endif
        if (!strcmp(qb->name, name)) {
            list_del(&qb->hook);
            free(qb->name);
            free(qb);
            close(qb->fds[0]);
            close(qb->fds[1]);
            q->branch_cnt--;
            return 0;
        }
    }
    return -1;
}

struct queue_branch *queue_branch_get(struct queue *q, const char *name)
{
    struct queue_branch *qb, *next;

#if defined (__linux__) || defined (__CYGWIN__)
    list_for_each_entry_safe(qb, next, &q->branch, hook) {
#elif defined (__WIN32__) || defined (WIN32) || defined (_MSC_VER)
    list_for_each_entry_safe(qb, struct queue_branch, next, struct queue_branch, &q->branch, hook) {
#endif
        if (!strcmp(qb->name, name)) {
            return qb;
        }
    }
    return NULL;
}

int queue_branch_notify(struct queue *q)
{
    struct queue_branch *qb, *next;
    char notify = '1';

#if defined (__linux__) || defined (__CYGWIN__)
    list_for_each_entry_safe(qb, next, &q->branch, hook) {
#elif defined (__WIN32__) || defined (WIN32) || defined (_MSC_VER)
    list_for_each_entry_safe(qb, struct queue_branch, next, struct queue_branch, &q->branch, hook) {
#endif

        if (write(qb->WR_FD, &notify, sizeof(notify)) != 1) {
            printf("write pipe failed: %s\n", strerror(errno));
        }
    }
    return 0;
}

struct item *queue_branch_pop(struct queue *q, const char *name)
{
    struct queue_branch *qb, *next;
    char notify = '1';

#if defined (__linux__) || defined (__CYGWIN__)
    list_for_each_entry_safe(qb, next, &q->branch, hook) {
#elif defined (__WIN32__) || defined (WIN32) || defined (_MSC_VER)
    list_for_each_entry_safe(qb, struct queue_branch, next, struct queue_branch, &q->branch, hook) {
#endif

        if (!strcmp(qb->name, name)) {
            if (read(qb->RD_FD, &notify, sizeof(notify)) != 1) {
                printf("read pipe failed: %s\n", strerror(errno));
            }
            return queue_pop(q);
        }
    }
    return NULL;
}
