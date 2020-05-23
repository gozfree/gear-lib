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

#if defined (__linux__) || defined (__CYGWIN__)
/******************************************************************************
 *
 ******************************************************************************/
#define OS_LINUX






#elif defined (__WIN32__) || defined (WIN32) || defined (_MSC_VER)
/******************************************************************************
 *
 ******************************************************************************/
#define OS_WINDOWS
#include "libposix4win.h"






#elif defined (__APPLE__)
/******************************************************************************
 *
 ******************************************************************************/
#define OS_APPLE






#elif defined (__ANDROID__)
/******************************************************************************
 *
 ******************************************************************************/
#define OS_ANDROID





#elif defined (FREERTOS) || defined (THREADX)
/******************************************************************************
 *
 ******************************************************************************/
#define OS_RTOS
#include "libposix4rtos.h"

#endif


#ifdef __cplusplus
}
#endif
#endif
