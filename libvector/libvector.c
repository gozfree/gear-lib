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
#include "libvector.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#define VECTOR_DEFAULT_BUF_LEN  (1024)

void _vector_push_back(struct vector *v, void *e, size_t type_size)
{
    size_t resize;
    void *pnew;
    if (!v || !e || type_size != v->type_size) {
        printf("%s: paraments invalid!\n", __func__);
        return;
    }
check:
    if ((v->size + 1) * v->type_size >= v->capacity) {
        resize = v->capacity + VECTOR_DEFAULT_BUF_LEN;
        pnew = realloc(v->buf.iov_base, resize);
        if (!pnew) {
            printf("realloc failed!\n");
            return;
        }
        v->buf.iov_base = pnew;
        v->buf.iov_len = resize;
        v->capacity = resize;
        goto check;
    }
    void *ptop = (uint8_t *)v->buf.iov_base + v->size * v->type_size;
    memcpy(ptop, e, v->type_size);
    v->size++;
}

void vector_pop_back(struct vector *v)
{
    if (!v) {
        printf("%s: paraments invalid!\n", __func__);
        return;
    }
    if (v->size <= 0) {
        printf("vector is empty already, cannot pop!\n");
        return;
    }
    v->size--;
}

int vector_empty(struct vector *v)
{
    if (!v) {
        printf("%s: paraments invalid!\n", __func__);
        return -1;
    }
    v->tmp_cursor = 0;
    return (v->size == 0);
}

vector_iter vector_begin(struct vector *v)
{
    if (!v) {
        printf("%s: paraments invalid!\n", __func__);
        return NULL;
    }
    return v->buf.iov_base;
}

vector_iter vector_end(struct vector *v)
{
    if (!v) {
        printf("%s: paraments invalid!\n", __func__);
        return NULL;
    }
    return (void *)((uint8_t *)v->buf.iov_base + v->size * v->type_size);
}

vector_iter vector_last(struct vector *v)
{
    if (!v) {
        printf("%s: paraments invalid!\n", __func__);
        return NULL;
    }
    return (void *)((uint8_t *)v->buf.iov_base + (v->size-1) * v->type_size);
}

vector_iter vector_next(struct vector *v)
{
    if (!v) {
        printf("%s: paraments invalid!\n", __func__);
        return NULL;
    }
    if (v->tmp_cursor < v->size) {
        v->tmp_cursor++;
    } else {
        return NULL;
    }
    return (void *)((uint8_t *)v->buf.iov_base + v->tmp_cursor * v->type_size);
}

vector_iter vector_prev(struct vector *v)
{
    if (!v) {
        printf("%s: paraments invalid!\n", __func__);
        return NULL;
    }
    v->tmp_cursor--;
    return (void *)((uint8_t *)v->buf.iov_base + v->tmp_cursor * v->type_size);
}

void *_vector_iter_value(struct vector *v, vector_iter iter)
{
    if (!v || !iter) {
        printf("%s: paraments invalid!\n", __func__);
        return NULL;
    }
    return (void *)((uint8_t *)v->buf.iov_base + v->tmp_cursor * v->type_size);
}

void *_vector_at(struct vector *v, int pos)
{
    if (!v || pos < 0) {
        printf("%s: paraments invalid!\n", __func__);
        return NULL;
    }
    return (void *)((uint8_t *)v->buf.iov_base + pos * v->type_size);
}

struct vector *_vector_create(size_t size)
{
    struct vector *v = (struct vector *)calloc(1, sizeof(struct vector));
    if (!v) {
        printf("malloc vector failed!\n");
        return NULL;
    }
    v->size = 0;
    v->tmp_cursor = 0;
    v->type_size = size;
    v->max_size = (size_t)(-1/size);
    v->capacity = VECTOR_DEFAULT_BUF_LEN;
    v->buf.iov_len = VECTOR_DEFAULT_BUF_LEN;
    v->buf.iov_base = calloc(1, v->buf.iov_len);
    if (!v->buf.iov_base) {
        printf("malloc vector buf failed!\n");
        goto failed;
    }
    return v;
failed:
    if (v) {
        free(v);
    }
    return NULL;
}

void vector_destroy(struct vector *v)
{
    if (!v) {
        return;
    }
    if (v->buf.iov_base) {
        free(v->buf.iov_base);
    }
    free(v);
}
