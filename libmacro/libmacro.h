/******************************************************************************
 * Copyright (C) 2014-2015
 * file:    libmacro.h
 * author:  gozfree <gozfree@163.com>
 * created: 2016-06-29 11:07:41
 * updated: 2016-06-29 11:07:41
 *****************************************************************************/
#ifndef LIBMACRO_H
#define LIBMACRO_H

#include "kernel_list.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * MACRO DEFINES ARE UPPERCASE
 */

#ifdef __GNUC__
#define LIKELY(x)           (__builtin_expect(!!(x), 1))
#define UNLIKELY(x)         (__builtin_expect(!!(x), 0))
#else
#define LIKELY(x)           (x)
#define UNLIKELY(x)         (x)
#endif

#define SWAP(a, b)          \
    do { typeof(a) __tmp = (a); (a) = (b); (b) = __tmp; } while (0)

#define MIN(a, b)           ((a) > (b) ? (b) : (a))
#define MAX(a, b)           ((a) > (b) ? (a) : (b))

#define CALLOC(size, type)  (type *)calloc(size, sizeof(type))
#define SIZEOF(array)       (sizeof(array)/sizeof(array[0]))

#define VERBOSE()           \
    do {\
        printf("%s:%s:%d xxxxxx\n", __FILE__, __func__, __LINE__);\
    } while (0);

#define DUMP_BUFFER(buf, len)                                   \
    do {                                                        \
        int _i;                                                 \
        if (buf == NULL || len <= 0) {                          \
            break;                                              \
        }                                                       \
        for (_i = 0; _i < len; _i++) {                          \
            if (!(_i%16))                                       \
                printf("\n%p: ", buf+_i);                       \
            printf("%02x ", (*((char *)buf + _i)) & 0xff);      \
        }                                                       \
        printf("\n");                                           \
    } while (0)




#ifdef __cplusplus
}
#endif
#endif
