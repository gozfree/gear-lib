/******************************************************************************
 * Copyright (C) 2014-2017 Zhifeng Gong <gozfree@163.com>
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

#define CALLOC(size, type)  (type *)calloc(size, sizeof(type))

void *memdup(void *src, size_t len)
{
    void *dst = calloc(1, len);
    if (dst) {
        memcpy(dst, src, len);
    }
    return dst;
}

struct item *item_alloc(void *data, size_t len)
{
    struct item *item = CALLOC(1, struct item);
    if (!item) {
        printf("malloc failed!\n");
        return NULL;
    }
    item->data.iov_base = memdup(data, len);
    item->data.iov_len = len;
    return item;
}

void item_free(struct item *item)
{
    if (!item) {
        return;
    }
    free(item->data.iov_base);
    free(item);
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
    return q;
}

int queue_flush(struct queue *q)
{
    struct item *item, *next;
    pthread_mutex_lock(&q->lock);
    list_for_each_entry_safe(item, next, &q->head, entry) {
        list_del(&item->entry);
        item_free(item);
    }
    q->depth = 0;
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

int queue_push(struct queue *q, struct item *item)
{
    if (!q || !item) {
        printf("invalid paraments!\n");
        return -1;
    }
    if (q->depth > q->max_depth) {
        queue_flush(q);
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
