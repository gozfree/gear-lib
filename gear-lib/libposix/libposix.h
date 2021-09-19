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
#ifndef LIBPOSIX_H
#define LIBPOSIX_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LIBPOSIX_VERSION "0.1.1"

/******************************************************************************
 * OS_LINUX
 ******************************************************************************/
#if defined (__linux__) || defined (__CYGWIN__)
#define OS_LINUX
#include <stdbool.h>
#include <stdint.h>
#include <sys/uio.h>
#include "kernel_list.h"

#define GEAR_API __attribute__((visibility("default")))

/******************************************************************************
 * OS_WINDOWS
 ******************************************************************************/
#elif defined (__WIN32__) || defined (WIN32) || defined (_MSC_VER)
#define OS_WINDOWS
#define GEAR_API __declspec(dllexport)

#include "libposix4win.h"
#include "kernel_list_win32.h"


/******************************************************************************
 * OS_APPLE
 ******************************************************************************/
#elif defined (__APPLE__)
#define OS_APPLE
#include <stdbool.h>
#include <sys/uio.h>
#include "kernel_list.h"
#define GEAR_API

/******************************************************************************
 * OS_ANDROID
 ******************************************************************************/
#elif defined (__ANDROID__)
#define OS_ANDROID
#include <stdbool.h>
#define GEAR_API

/******************************************************************************
 * OS_RTOS
 ******************************************************************************/
#elif defined (FREERTOS) || defined (THREADX)
#define OS_RTOS
#include "libposix4rtos.h"
#include "kernel_list.h"
#define GEAR_API

#elif defined (RT_USING_POSIX)
#define OS_RTTHREAD
#include <stdbool.h>
#include "libposix4rtthread.h"
#include "kernel_list.h"
#define GEAR_API

#else
#error "OS_UNDEFINED"
#endif

#include "libposix_ext.h"

#ifdef __cplusplus
}
#endif
#endif
