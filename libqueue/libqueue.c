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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include "libqueue.h"

#define QUEUE_MAX_DEPTH 200

struct item *item_alloc(struct queue *q, void *data, size_t len)
{
    struct item *item = CALLOC(1, struct item);
    if (!item) {
        printf("malloc failed!\n");
        return NULL;
    }
    if (q->alloc_hook) {
        item->opaque = (q->alloc_hook)(data, len);
    } else {
        item->data.iov_base = memdup(data, len);
        item->data.iov_len = len;
    }
    return item;
}

void item_free(struct queue *q, struct item *item)
{
    if (!item) {
        return;
    }
    if (q->free_hook) {
        (q->free_hook)(item->opaque);
    } else {
        free(item->data.iov_base);
    }
    free(item);
}

int queue_set_mode(struct queue *q, enum queue_mode mode)
{
    q->mode = mode;
    return 0;
}

int queue_set_hook(struct queue *q, alloc_hook *alloc_cb, free_hook *free_cb)
{
    q->alloc_hook = alloc_cb;
    q->free_hook = free_cb;
    return 0;
}

int queue_set_depth(struct queue *q, int depth)
{
    q->max_depth = depth;
    return 0;
}

struct queue *queue_create()
{
    struct queue *q = CALLOC(1, struct queue);
    if (!q) {
        printf("malloc failed!\n");
        return NULL;
    }
    INIT_LIST_HEAD(&q->head);
    pthread_mutex_init(&q->lock, NULL);
    pthread_cond_init(&q->cond, NULL);
    q->depth = 0;
    q->max_depth = QUEUE_MAX_DEPTH;
    q->mode = QUEUE_FULL_FLUSH;
    q->alloc_hook = NULL;
    q->free_hook = NULL;
    return q;
}

int queue_flush(struct queue *q)
{
    struct item *item, *next;
    pthread_mutex_lock(&q->lock);
    list_for_each_entry_safe(item, next, &q->head, entry) {
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
    pthread_cond_signal(&q->cond);
    pthread_mutex_unlock(&q->lock);
    if (q->depth > q->max_depth) {
        printf("queue depth reach max depth %d\n", q->depth);
    }
    //printf("push queue depth is %d\n", q->depth);
    return 0;
}

struct item *queue_pop(struct queue *q)
{
    if (!q) {
        printf("invalid parament!\n");
        return NULL;
    }

    struct item *item = NULL;
    pthread_mutex_lock(&q->lock);
    while (list_empty(&q->head)) {
        struct timeval now;
        struct timespec outtime;
        gettimeofday(&now, NULL);
        outtime.tv_sec = now.tv_sec + 1;
        outtime.tv_nsec = now.tv_usec * 1000;
        int ret = pthread_cond_timedwait(&q->cond, &q->lock, &outtime);
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
        list_del(&item->entry);
        --(q->depth);
    }
    pthread_mutex_unlock(&q->lock);
    //printf("pop queue depth is %d\n", q->depth);
    return item;
}

int queue_get_depth(struct queue *q)
{
    return q->depth;
}
