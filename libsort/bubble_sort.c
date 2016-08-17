/*****************************************************************************
 * Copyright (C) 2014-2015
 * file:    bubble_sort.c
 * author:  gozfree <gozfree@163.com>
 * created: 2015-08-09 00:43
 * updated: 2015-08-09 00:43
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "libsort.h"

static void generic_swap(void *a, void *b, int size)
{
    char t;
    do {
        t = *(char *)a;
        *(char *)a++ = *(char *)b;
        *(char *)b++ = t;
    } while (--size > 0);
}

static int u32_cmp(const void *a, const void *b)
{
    return *(int *)a - *(int *)b;
}


#define bsort(type, array, len) \
    do {\
        size_t _i = 0, _j = 0;\
        for (_i = 0; _i < len; ++_i) {\
            for (_j = 0; _j < len-_i-1; ++_j) {\
                if (*((type *)array+_j) > *((type *)array+_j+1)) {\
                    generic_swap(((type *)array+_j), ((type *)array+_j+1), sizeof(type));\
                }\
            }\
        }\
    } while (0)

void bubble_sortf(float *array, size_t len)
{
    if (!array || len <=0) {
        printf("invalid parament\n");
        return;
    }
    bsort(float, array, len);
}

void bubble_sort(void *array, size_t num, size_t size)
{
    if (!array || num <=0) {
        printf("invalid parament\n");
        return;
    }
    int i = 0, j = 0;
    for (i = 0; i < num; ++i) {
        for (j = 0; j < num-i-1; ++j) {
            if ((u32_cmp(array+j*size, array+(j+1)*size)) > 0) {
                generic_swap(array+j*size, array+(j+1)*size, size);
            }
        }
    }
}
