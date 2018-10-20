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
#include <stdbool.h>
#if defined (__linux__)
#include <sys/uio.h>
#elif defined (__WIN32__)
#include "win.h"
#endif
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

#define UNUSED(arg)  arg = arg


void *dl_override(const char *name);

/*
 * using HOOK_CALL(func, args...), prev/post functions can be hook into func
 */
#define HOOK_CALL(func, ...) \
    ({ \
        func##_prev(__VA_ARGS__); \
        __typeof__(func) *sym = dl_override(#func); \
        sym(__VA_ARGS__); \
        func##_post(__VA_ARGS__); \
    })

/*
 * using CALL(func, args...), you need override api
 */
#define CALL(func, ...) \
    ({__typeof__(func) *sym = (__typeof__(func) *)dl_override(#func); sym(__VA_ARGS__);}) \


int is_little_endian(void);
int system_noblock(char **argv);
ssize_t system_with_result(const char *cmd, void *buf, size_t count);
ssize_t system_noblock_with_result(char **argv, void *buf, size_t count);

bool proc_exist(const char *proc);

#ifdef __cplusplus
}
#endif
#endif
