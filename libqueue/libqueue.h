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
#ifndef LIBQUEUE_H
#define LIBQUEUE_H

#include <sys/uio.h>
#include <pthread.h>
#include <libmacro.h>


#ifdef __cplusplus
extern "C" {
#endif

struct item {
    struct iovec data;
    struct list_head entry;
};

struct queue {
    struct list_head head;
    int depth;
    int max_depth;
    pthread_mutex_t lock;
    pthread_cond_t cond;
};

struct item *item_alloc(void *data, size_t len);
void item_free(struct item *item);

struct queue *queue_create();
void queue_destroy(struct queue *q);
struct item *queue_pop(struct queue *q);
int queue_push(struct queue *q, struct item *item);
int queue_flush(struct queue *q);
int queue_get_depth(struct queue *q);


#ifdef __cplusplus
}
#endif

#endif
