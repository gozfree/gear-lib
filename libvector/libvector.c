/*****************************************************************************
 * Copyright (C) 2014-2015
 * file:    libvector.c
 * author:  gozfree <gozfree@163.com>
 * created: 2015-09-16 00:49
 * updated: 2015-09-16 00:49
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <libgzf.h>
#include "libvector.h"

#define VECTOR_DEFAULT_BUF_LEN  (1024)

void push_back(struct vector *v, void *e)
{
    size_t resize;
    void *pnew;
    CHECK_INVALID_PARAMENT(!v || !e);
    v->size++;
    if (v->size * v->type_size >= v->capacity) {
        resize = v->capacity + VECTOR_DEFAULT_BUF_LEN;
        pnew = realloc(v->buf.iov_base, resize);
        if (!pnew) {
            printf("realloc failed!\n");
            return;
        }
        v->buf.iov_base = pnew;
        v->capacity += resize;
    }
    void *ptop = v->buf.iov_base + v->size * v->type_size;
    memcpy(ptop, e, v->type_size);
    printf("v->size = %zu, v->val = %d\n", v->size, *(int *)e);
}

void vector_pop_back(struct vector *v)
{
    CHECK_INVALID_PARAMENT(!v);
    v->size--;
    printf("v->size = %zu\n", v->size);
}

bool vector_empty(struct vector *v)
{
    CHECK_INVALID_PARAMENT_WITH_RETURN(!v, true);
    return (v->size == 0);
}

void *begin(struct vector *v)
{
    CHECK_INVALID_PARAMENT_WITH_RETURN(!v, NULL);
    return v->buf.iov_base;
}

void *end(struct vector *v)
{
    CHECK_INVALID_PARAMENT_WITH_RETURN(!v, NULL);
    return v->buf.iov_base + v->size * v->type_size;
}

void *plusplus(struct vector *v)
{
    CHECK_INVALID_PARAMENT_WITH_RETURN(!v, NULL);
    v->tmp_cursor++;
    return v->buf.iov_base + v->tmp_cursor * v->type_size;
}

struct vector *init(type_arg_t ta, size_t size)
{
    struct vector *v = CALLOC(1, struct vector);
    v->type = ta;
    v->size = 0;
    v->tmp_cursor = 0;
    v->type_size = size;
    v->max_size = (size_t)(-1/size);
    v->capacity = VECTOR_DEFAULT_BUF_LEN;
    IOVEC_INIT(v->buf, VECTOR_DEFAULT_BUF_LEN);
    switch (ta) {
    case _int:
        printf("init int\n");
        break;
    case _long:
        printf("init long\n");
        break;
    default:
        break;
    }
#if 0
    printf("type = %d\n", v->type);
    printf("size = %zu\n", v->size);
    printf("type_size = %zu\n", v->type_size);
    printf("max_size = %zu\n", v->max_size);
    printf("capacity = %zu\n", v->capacity);
#endif
    return v;
}
