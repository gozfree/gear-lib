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
#include "libvector.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define VECTOR_DEFAULT_BUF_LEN  (1024)

void _vector_push_back(struct vector *v, void *e, size_t type_size)
{
    size_t resize;
    void *pnew;
    void *ptop;
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
    ptop = (uint8_t *)v->buf.iov_base + v->size * v->type_size;
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
