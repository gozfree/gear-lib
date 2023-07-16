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
#ifndef LIBPOSIX4WIN_H
#define LIBPOSIX4WIN_H

#ifdef ENV_MINGW
#error the msvclibx should not be include in mingw env
#endif

#include "msvclibx.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <tlhelp32.h>
#include <windows.h>
#include "pthreads4w/pthread.h"
#include "pthreads4w/semaphore.h"
#endif
#include "kernel_list_win32.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * basic types
 ******************************************************************************/
#define inline                    __inline
#define __func__                  __FUNCTION__

/******************************************************************************
 * I/O string APIs
 ******************************************************************************/
#define sprintf                   _sprintf
#define strcasecmp                _stricmp
#define strncasecmp               _strnicmp
#define strdup                    _strdup

#define PATH_SPLIT                '\\'

#define iovec                     _WSABUF
#define iov_len                   len
#define iov_base                  buf
GEAR_API ssize_t writev(int fd, const struct iovec *iov, int iovcnt);

GEAR_API char *dup_wchar_to_utf8(wchar_t *w);


/******************************************************************************
 * sys file APIs
 ******************************************************************************/
#define MAXPATHLEN                1024

/******************************************************************************
 * pthread APIs
 ******************************************************************************/
#define gettid                    GetCurrentThreadId


/******************************************************************************/
#define dlopen(name, flags) win32_dlopen(name)
#define dlclose FreeLibrary
#define dlsym GetProcAddress


/******************************************************************************
 * time APIs
 ******************************************************************************/
struct win_time_t
{
    int wday;/** This represents day of week where value zero means Sunday */
    int day;/** This represents day of month: 1-31 */
    int mon;/** This represents month, with the value is 0 - 11 (zero is January) */
    /** This represent the actual year (unlike in ANSI libc where
     *  the value must be added by 1900).
     */
    int year;

    int sec;/** This represents the second part, with the value is 0-59 */
    int min;/** This represents the minute part, with the value is: 0-59 */
    int hour;/** This represents the hour part, with the value is 0-23 */
    int msec;/** This represents the milisecond part, with the value is 0-999 */
};

#ifndef  _TIMEZONE_DEFINED
struct timezone
{
    int tz_minuteswest;
    int tz_dsttime;
};
#endif

#define timeradd(tvp, uvp, vvp) \
        do { \
                (vvp)->tv_sec = (tvp)->tv_sec + (uvp)->tv_sec; \
                (vvp)->tv_usec = (tvp)->tv_usec + (uvp)->tv_usec; \
                if ((vvp)->tv_usec >= 1000000) { \
                        (vvp)->tv_sec++; \
                        (vvp)->tv_usec -= 1000000; \
                } \
        } while (0)

#define timersub(tvp, uvp, vvp) \
        do { \
                (vvp)->tv_sec = (tvp)->tv_sec - (uvp)->tv_sec; \
                (vvp)->tv_usec = (tvp)->tv_usec - (uvp)->tv_usec; \
                if ((vvp)->tv_usec < 0) { \
                        (vvp)->tv_sec--; \
                        (vvp)->tv_usec += 1000000; \
                } \
        } while (0)

#define localtime_r(timep,result) ERR_PTR(localtime_s(result, timep)==0?true:false)
#define sleep(n) Sleep(n*1000)
GEAR_API void usleep(unsigned long usec);

typedef int clockid_t;
#ifndef CLOCK_REALTIME
#define CLOCK_REALTIME           ((clockid_t)0)
#endif
#ifndef CLOCK_MONOTONIC
#define CLOCK_MONOTONIC          ((clockid_t)1)
#endif
#ifndef CLOCK_PROCESS_CPUTIME_ID
#define CLOCK_PROCESS_CPUTIME_ID ((clockid_t)2)
#endif
#ifndef CLOCK_THREAD_CPUTIME_ID
#define CLOCK_THREAD_CPUTIME_ID  ((clockid_t)3)
#endif



/******************************************************************************
 * driver IOC APIs
 ******************************************************************************/
#define _IOC_NRBITS     8
#define _IOC_TYPEBITS   8
#define _IOC_SIZEBITS   14
#define _IOC_DIRBITS    2

#define _IOC_NRMASK     ((1 << _IOC_NRBITS)-1)
#define _IOC_TYPEMASK   ((1 << _IOC_TYPEBITS)-1)
#define _IOC_SIZEMASK   ((1 << _IOC_SIZEBITS)-1)
#define _IOC_DIRMASK    ((1 << _IOC_DIRBITS)-1)

#define _IOC_NRSHIFT    0
#define _IOC_TYPESHIFT  (_IOC_NRSHIFT+_IOC_NRBITS)
#define _IOC_SIZESHIFT  (_IOC_TYPESHIFT+_IOC_TYPEBITS)
#define _IOC_DIRSHIFT   (_IOC_SIZESHIFT+_IOC_SIZEBITS)

#define _IOC_NONE       0U
#define _IOC_WRITE      1U
#define _IOC_READ       2U


#define _IOC(dir, type, nr, size) \
        (((dir)   << _IOC_DIRSHIFT) | \
        + ((type) << _IOC_TYPESHIFT) | \
        + ((nr)   << _IOC_NRSHIFT) | \
        + ((size) << _IOC_SIZESHIFT))


#define _IOWR(type,nr,size)  _IOC(_IOC_READ|_IOC_WRITE,(type),(nr),sizeof(size))


/******************************************************************************
 * socket APIs
 ******************************************************************************/

/******************************************************************************
 * system APIs
 ******************************************************************************/
GEAR_API int pipe(int fds[2]);
GEAR_API int pipe_read(int fd, void *buf, size_t len);
GEAR_API int pipe_write(int fd, const void *buf, size_t len);
GEAR_API void pipe_close(int fds[2]);

GEAR_API int eventfd(unsigned int initval, int flags);
#define eventfd_write   pipe_write
#define eventfd_read    pipe_read
GEAR_API void eventfd_close(int fd);

GEAR_API int get_nprocs();

/******************************************************************************
 * memory APIs
 ******************************************************************************/
#define memalign(align, size)     _aligned_malloc(size, align)


#ifdef __cplusplus
}
#endif
#endif
