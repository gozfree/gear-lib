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
#ifndef QUEUE_H
#define QUEUE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <libmacro.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

typedef struct {
    void *data;
    struct list_head entry;
} list_t;

typedef struct {
    list_t member;
    uint32_t length;
} queue_t;

static inline void queue_init(queue_t *q)
{
    if (q == NULL) {
        return;
    }
    INIT_LIST_HEAD(&q->member.entry);
    q->member.data = NULL;
    q->length = 0;
}

static inline void queue_clear(queue_t *q)
{
    if (q == NULL) {
        return;
    }
    list_t *iter = NULL;
    list_t *next = NULL;
    list_for_each_entry_safe(iter, next, &q->member.entry, entry) {
        list_del(&iter->entry);
        if (!iter) {
            continue;
        }
        free(iter->data);
        free(iter);
    }
    q->length = 0;
}

static inline void *queue_pop_head(queue_t *q)
{
    if (q == NULL) {
        return NULL;
    }
    list_t *h = list_first_entry_or_null(&q->member.entry, list_t, entry);
    if (h == NULL) {
        return NULL;
    }
    list_del(&h->entry);
    q->length--;
    return h->data;
}

static inline uint32_t queue_get_length(queue_t *q)
{
    if (q == NULL) {
        return 0;
    }
    return q->length;
}

static inline void *queue_peek_head(queue_t *q)
{
    if (q == NULL) {
        return NULL;
    }
    list_t *h = list_first_entry_or_null(&q->member.entry, list_t, entry);
    if (h == NULL) {
        return NULL;
    }
    return h->data;
}

static inline void *queue_peek_tail(queue_t *q)
{
    if (q == NULL) {
        return NULL;
    }
    list_t *h = list_last_entry_or_null(&q->member.entry, list_t, entry);
    if (h == NULL) {
        return NULL;
    }
    return h->data;
}

static inline void queue_push_tail(queue_t *q, void *data)
{
    if (q == NULL) {
        return;
    }
    list_t *_new = (list_t *)calloc(1, sizeof(list_t));
    _new->data = data;
    list_add_tail(&_new->entry, &q->member.entry);
    q->length++;
}

static inline list_t *queue_find(queue_t *q, const void *data)
{
    if (q == NULL) {
        return NULL;
    }
    list_t *iter;
    list_for_each_entry(iter, &q->member.entry, entry) {
        if (iter->data == data)
            break;
    }
    return iter;
}

static inline void queue_insert_after(queue_t *q, list_t *sibling, void *data)
{
    if (q == NULL || sibling == NULL) {
        return;
    }
    list_t *_new = (list_t *)calloc(1, sizeof(list_t));
    _new->data = data;
    list_add_tail(&_new->entry, &sibling->entry);
    q->length++;
}

static inline list_t *queue_peek_head_link(queue_t *q)
{
    if (q == NULL) {
        return NULL;
    }
    list_t *h = list_first_entry_or_null(&q->member.entry, list_t, entry);
    if (h == NULL) {
        return NULL;
    }
    return h;
}

static inline list_t *queue_peek_tail_link(queue_t *q)
{
    if (q == NULL) {
        return NULL;
    }
    list_t *h = list_last_entry_or_null(&q->member.entry, list_t, entry);
    if (h == NULL) {
        return NULL;
    }
    return h;
}


#ifdef __cplusplus
extern "C" {
#endif
#endif
