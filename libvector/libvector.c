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

void push_back(void *e)
{
    printf("%s:%d xxx\n", __func__, __LINE__);
}

struct vector *vector_init(size_t size)
{
    struct vector *v = CALLOC(1, struct vector);
    v->push_back = push_back;
    v->type_size = size;
    return v;
}
