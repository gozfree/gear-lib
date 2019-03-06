/*****************************************************************************
 * Copyright (C) 2014-2015
 * file:    bubble_sort.c
 * author:  gozfree <gozfree@163.com>
 * created: 2015-08-09 00:43
 * updated: 2015-08-09 00:43
 *****************************************************************************/
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define BUBBLE_SORT(type, array, len) \
    do { \
        type *s = (type *)array; \
        type *e = s + len - 1; \
        type *p, *q; \
        for (q = e; q > s; q--) { \
            for (p = s; p < q; p++) { \
                if (p[0] > p[1]) { \
                    type t = p[0]; \
                    p[0] = p[1]; \
                    p[1] = t; \
                } \
            } \
        } \
    } while (0)

void bubble_sortf(float *array, size_t len)
{
    if (!array || len <=0) {
        printf("invalid parament\n");
        return;
    }
    BUBBLE_SORT(float, array, len);
}

int bubble_sort(void *array, size_t num, size_t size, fp_cmp cmp)
{
    CHK_PARAMETERS(array, num, size, cmp);

    byte *s = (byte *)array;
    byte *e = s + size * (num - 1);

    byte *p, *q;
    for (q = e; q > s; q -= size) {
        for (p = s; p < q; p += size) {
            if (cmp(p, p + size, size) > 0) {
                byte_swap(p, p + size, size);
            }
        }
    }
    return 0;
}
