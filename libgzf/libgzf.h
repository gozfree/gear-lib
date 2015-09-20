/*****************************************************************************
 * Copyright (C) 2014-2015
 * file:    libgzf.h
 * author:  gozfree <gozfree@163.com>
 * created: 2015-05-13 01:28
 * updated: 2015-07-12 02:32
 *****************************************************************************/
#ifndef _LIBGZF_H_
#define _LIBGZF_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>
#include <sys/uio.h>
#include <kernel_list.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * c++ parament compatibility
 *****************************************************************************/
#ifndef bool
#define bool int
#endif

#ifndef true
#define true (1==1)
#endif

#ifndef false
#define false (0==1)
#endif


/******************************************************************************
 * instructions
 *****************************************************************************/
#ifdef __GNUC__
#define LIKELY(x)       (__builtin_expect(!!(x), 1))
#define UNLIKELY(x)     (__builtin_expect(!!(x), 0))
#else
#define LIKELY(x)       (x)
#define UNLIKELY(x)     (x)
#endif

/******************************************************************************
 * instructions
 *****************************************************************************/
/*
 * swap - swap value of @a and @b
 */
#define swap(a, b) \
    do { typeof(a) __tmp = (a); (a) = (b); (b) = __tmp; } while (0)

/******************************************************************************
 * math
 *****************************************************************************/
#define min(a, b)    ((a) > (b) ? (b) : (a))
#define max(a, b)    ((a) > (b) ? (a) : (b))

/******************************************************************************
 * memory
 *****************************************************************************/
#define CALLOC(size, type) \
    (type *)calloc(size, sizeof(type))
#define SIZEOF(array)   \
    sizeof(array)/sizeof(array[0])
#define IOVEC_INIT(buf, len)            \
    do {                                \
        buf.iov_len = len;              \
        buf.iov_base = calloc(1, len);  \
    } while (0)

/******************************************************************************
 * debug
 *****************************************************************************/
#define VERBOSE() \
    do {\
        fprintf(stderr, "%s:%s:%d xxxxxx\n", __FILE__, __func__, __LINE__);\
    } while (0);
#define print_buffer(buf, len)                                  \
    do {                                                        \
        int _i;                                                 \
        if (!buf || len <= 0) {                                 \
            break;                                              \
        }                                                       \
        for (_i = 0; _i < len; _i++) {                          \
            if (!(_i%16))                                       \
                printf("\n%p: ", buf+_i);                       \
            printf("%02x ", (*((char *)buf + _i)) & 0xff);      \
        }                                                       \
        printf("\n");                                           \
    } while (0)


/******************************************************************************
 * color
 *****************************************************************************/
#define FG_BLACK        30
#define FG_RED          31
#define FG_GREEN        32
#define FG_YELLOW       33
#define FG_BLUE         34
#define FG_MAGENTA      35
#define FG_CYAN         36
#define FG_WHITE        37
#define BG_BLACK        40
#define BG_RED          41
#define BG_GREEN        42
#define BG_YELLOW       43
#define BG_BLUE         44
#define BG_MAGENTA      45
#define BG_CYAN         46
#define BG_WHITE        47
#define B_RED(str)      "\033[1;31m"str"\033[0m"
#define B_GREEN(str)    "\033[1;32m"str"\033[0m"
#define B_YELLOW(str)   "\033[1;33m"str"\033[0m"
#define B_BLUE(str)     "\033[1;34m"str"\033[0m"
#define B_MAGENTA(str)  "\033[1;35m"str"\033[0m"
#define B_CYAN(str)     "\033[1;36m"str"\033[0m"
#define B_WHITE(str)    "\033[1;37m"str"\033[0m"
#define RED(str)        "\033[31m"str"\033[0m"
#define GREEN(str)      "\033[32m"str"\033[0m"
#define YELLOW(str)     "\033[33m"str"\033[0m"
#define BLUE(str)       "\033[34m"str"\033[0m"
#define MAGENTA(str)    "\033[35m"str"\033[0m"
#define CYAN(str)       "\033[36m"str"\033[0m"
#define WHITE(str)      "\033[37m"str"\033[0m"



#ifdef __cplusplus
}
#endif
#endif
