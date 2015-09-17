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

/*
#define vector_push_back(v, e)	\
    (__builtin_types_compatible_p(typeof(e), int) \
    ? _push_back(v, (void *)&e) \
    : __builtin_types_compatible_p(typeof(e), long) \
    ? push_back_long(v, e) \
    : __builtin_types_compatible_p(typeof(e), float) \
    ? push_back_float(v, e)	\
    : printf("not support type\n"))
*/



void push_back(struct vector *v, void *e)
{
    printf("%s:%d xxx\n", __func__, __LINE__);
}

void pop_back(struct vector *v)
{
    printf("%s:%d xxx\n", __func__, __LINE__);
}

int back_int(struct vector *v)
{
    printf("%s:%d xxx\n", __func__, __LINE__);
    return 0;
}


struct vector *vector_init(size_t size)
{
    struct vector *v = CALLOC(1, struct vector);
//    v->push_back = push_back;
    v->type_size = size;
    IOVEC_INIT(v->buf, VECTOR_DEFAULT_BUF_LEN);
    return v;
}
