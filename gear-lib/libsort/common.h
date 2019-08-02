/*****************************************************************************
 * file:    common.h
 * author:  xw_y_am <xw_y_am@163.com>
 * created: 2019-03-03
 * description: libsort inner common functions
 *****************************************************************************/

#ifndef COMMON_H
#define COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

#include "libsort.h"
#include <stdio.h>

typedef unsigned char byte;

#define CHK_PARAMETERS(array, num, size, cmp) \
    do { \
        if (!array || !size) { \
            printf("invalid parameter!\n"); \
            return -1; \
        } \
        if (num < 2) { \
            printf("invalid parameter!\n"); \
            return -1; \
        } \
        if (num >> 20) { \
            printf("cannot sort array with more than 1048576 elements!\n"); \
            return -1; \
        } \
        if (!cmp) cmp = default_cmp; \
    } while (0)

static inline void byte_swap(byte *p, byte *q, size_t size)
{
    for (; size--; p++, q++) {
        byte t = *p;
        *p = *q;
        *q = t;
    }
}

static int default_cmp(const void *a, const void *b, size_t size)
{
    const byte *p = (const byte *)a;
    const byte *q = (const byte *)b;
    for (; size--; p++, q++) {
        if (*p != *q) return *p - *q;
    }
    return 0;
}

#ifdef __cplusplus
}
#endif
#endif 
