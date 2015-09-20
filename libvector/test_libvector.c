/*****************************************************************************
 * Copyright (C) 2014-2015
 * file:    test_libvector.c
 * author:  gozfree <gozfree@163.com>
 * created: 2015-05-31 19:42
 * updated: 2015-05-31 19:42
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "libvector.h"

int main(int argc, char **argv)
{
    int sum = 0;
    int i;
    int t1 = 100, t2 = 200, t3 = 300;
    vector_t *a = vector_new(int);
    vector_push_back(a, t1);
    vector_push_back(a, t2);
    vector_push_back(a, t3);
    for (i = vector_begin(a, int); i != vector_end(a, int); i = vector_plusplus(a, int)) {
        printf("vector member: %d\n", i);
    }
    while (!vector_empty(a)) {
        sum += vector_back(a, int);
        vector_pop_back(a);
    }
    printf("sum is %d\n", sum);
    printf("type = %d\n", a->type);
    printf("size = %zu\n", a->size);
    printf("type_size = %zu\n", a->type_size);
    printf("max_size = %zu\n", a->max_size);
    printf("capacity = %zu\n", a->capacity);
    return 0;
}
