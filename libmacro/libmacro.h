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
#include <stdlib.h>
#include <string.h>

#if defined (__linux__) || defined (__CYGWIN__) || defined (__APPLE__)
#include <stdbool.h>
#include <sys/uio.h>
#include "kernel_list.h"
#elif defined (__WIN32__) || defined (WIN32) || defined (_MSC_VER)
#include "win.h"
#include "kernel_list_win32.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
 * MACRO DEFINES ARE UPPERCASE
 */

/**
 * Variable-argument unused annotation
 */
#define UNUSED(e, ...)      (void) ((void) (e), ##__VA_ARGS__)

#ifdef __GNUC__
#define LIKELY(x)           (__builtin_expect(!!(x), 1))
#define UNLIKELY(x)         (__builtin_expect(!!(x), 0))
#else
#define LIKELY(x)           (x)
#define UNLIKELY(x)         (x)
#endif

#define SWAP(a, b)          \
    do { typeof(a) __tmp = (a); (a) = (b); (b) = __tmp; } while (0)

#define MIN2(a, b)          ((a) > (b) ? (b) : (a))
#define MAX2(a, b)          ((a) > (b) ? (a) : (b))
#define ABS(x)              ((x) >= 0 ? (x) : -(x))

#define CALLOC(size, type)  (type *) calloc(size, sizeof(type))
#define ARRAY_SIZE(a)       (sizeof(a) / sizeof(a[0]))

#define VERBOSE()                                                   \
    do {                                                            \
        printf("%s:%s:%d xxxxxx\n", __FILE__, __func__, __LINE__);  \
    } while (0)

#define DUMP_BUFFER(buf, len)                                            \
    do {                                                                 \
        int _i, _j=0;                                                    \
        char _tmp[128] = {0};                                             \
        if (buf == NULL || len <= 0) {                                   \
            break;                                                       \
        }                                                                \
        for (_i = 0; _i < len; _i++) {                                   \
            if (!(_i%16)) {                                              \
                if (_i != 0) {                                           \
                    printf("%s", _tmp);                                  \
                }                                                        \
                memset(_tmp, 0, sizeof(_tmp));                           \
                _j = 0;                                                  \
                _j += snprintf(_tmp+_j, 64, "\n%p: ", buf+_i);           \
            }                                                            \
            _j += snprintf(_tmp+_j, 4, "%02hhx ", *((char *)buf + _i));  \
        }                                                                \
        printf("%s\n", _tmp);                                            \
    } while (0)

#define ALIGN2(x, a)	(((x) + (a) - 1) & ~((a) - 1))

#define is_alpha(c) (((c) >=  'a' && (c) <= 'z') || ((c) >=  'A' && (c) <= 'Z'))

/**
 * Compile-time strlen(3)
 * XXX: Should only used for `char[]'  NOT `char *'
 * Assume string ends with null byte('\0')
 */
#define STRLEN(s)          (sizeof(s) - 1)

/**
 * Compile-time assurance  see: linux/arch/x86/boot/boot.h
 * Will fail build if condition yield true
 */
#ifndef BUILD_BUG_ON
#if defined (__WIN32__) || defined (_WIN32) || defined (_MSC_VER)
/*
 * MSVC compiler allows negative array size(treat as unsigned value)
 *  yet them don't allow zero-size array
 */
#define BUILD_BUG_ON(cond)      ((void) sizeof(char[!(cond)]))
#else
#define BUILD_BUG_ON(cond)      ((void) sizeof(char[1 - 2 * !!(cond)]))
#endif
#endif

void *memdup(void *src, size_t len);
struct iovec *iovec_create(size_t len);
void iovec_destroy(struct iovec *);

bool is_little_endian(void);

#ifdef __cplusplus
}
#endif
#endif
