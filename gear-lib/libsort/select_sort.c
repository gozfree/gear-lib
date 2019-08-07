/*****************************************************************************
 * Copyright (C) 2019-2022
 * file:    select_sort.c
 * author:  xw_y_am <xw_y_am@163.com>
 * created: 2019-03-06 00:01
 *****************************************************************************/
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>


int select_sort(void *array, size_t num, size_t size, fp_cmp cmp)
{
    CHK_PARAMETERS(array, num, size, cmp);

    byte *s = (byte *)array;
    byte *e = s + (num - 1) * size;

    for (; s < e; s += size) {
        byte *first = s;
        byte *p = first + 1;
        for (; p <= e; p++) {
            if (cmp(p, first, size) < 0) {
                first = p;
            }
        }
        if (first != s) {
            byte_swap(first, s, size);
        }
    }

    return 0;
}
