/******************************************************************************
 * Copyright (C) 2014-2018 Zhifeng Gong <gozfree@163.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with libraries; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 ******************************************************************************/
#ifndef LIBMACRO_H
#define LIBMACRO_H

#include <stdio.h>
#include <sys/uio.h>
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

#define MIN2(a, b)           ((a) > (b) ? (b) : (a))
#define MAX2(a, b)           ((a) > (b) ? (a) : (b))

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

#define ALIGN(x, a)	(((x) + (a) - 1) & ~((a) - 1))

void *memdup(void *src, size_t len);
struct iovec *iovec_create(size_t len);
void iovec_destroy(struct iovec *);


#ifdef __cplusplus
}
#endif
#endif
