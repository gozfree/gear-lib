/******************************************************************************
 * Copyright (C) 2014-2020 Zhifeng Gong <gozfree@163.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
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


bool is_little_endian(void);

#define is_character(c) \
    (((c)<='z'&&(c)>='a') || ((c)<='Z'&&(c)>='A'))

int system_noblock(char **argv);
ssize_t system_with_result(const char *cmd, void *buf, size_t count);
ssize_t system_noblock_with_result(char **argv, void *buf, size_t count);

bool proc_exist(const char *proc);

#ifdef __cplusplus
}
#endif
#endif
