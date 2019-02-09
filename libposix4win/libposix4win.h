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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <direct.h>
#include <winsock2.h>
#include <windows.h>
#include <process.h>
#include <tlhelp32.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * basic types
 ******************************************************************************/
#define false                     0
#define true                      1
typedef int                       bool;

#define inline                    __inline
#define __func__                  __FUNCTION__

typedef SSIZE_T                   ssize_t;
typedef SSIZE_T                   off_t;


/******************************************************************************
 * I/O string APIs
 ******************************************************************************/
#define snprintf                  _snprintf
#define sprintf                   _sprintf
#define strcasecmp                _stricmp
#define strdup                    _strdup

#define PATH_SPLIT                '\\'
#define PRId8                     "hhd"
#define PRId16                    "hd"
#define PRId32                    "ld"
#define PRId64                    "lld"
#define PRIu8                     "hhu"
#define PRIu16                    "hu"
#define PRIu32                    "lu"
#define PRIu64                    "llu"

#define iovec                     _WSABUF
#define iov_len                   len
#define iov_base                  buf
ssize_t writev(int fd, const struct iovec *iov, int iovcnt);


/******************************************************************************
 * sys file APIs
 ******************************************************************************/
#define STDIN_FILENO              0       /* standard input file descriptor */
#define STDOUT_FILENO             1       /* standard output file descriptor */
#define STDERR_FILENO             2       /* standard error file descriptor */
#define MAXPATHLEN                1024

typedef int                       gid_t;
typedef int                       uid_t;
typedef int                       dev_t;
typedef int                       ino_t;
typedef int                       mode_t;
typedef int                       caddr_t;

#define F_OK                      0
#define R_OK                      4
#define W_OK                      2
#define X_OK                      1

struct stat {
    dev_t         st_dev;
    ino_t         st_ino;
    mode_t        st_mode;
    //nlink_t       st_nlink;
    uid_t         st_uid;
    gid_t         st_gid;
    dev_t         st_rdev;
    off_t         st_size;
    unsigned long st_blksize;
    unsigned long st_blocks;
    time_t        st_atime;
};

int stat(const char *file, struct stat *buf);
int access(const char *pathname, int mode);
#define mkdir(path,mode)          _mkdir(path)
#define getcwd(buf, size)         GetModuleFileName(NULL, buf, size)


/******************************************************************************
 * pthread APIs
 ******************************************************************************/

#define pthread_once_t            INIT_ONCE
#define PTHREAD_ONCE_INIT         INIT_ONCE_STATIC_INIT
#define pthread_mutex_t           SRWLOCK
#define pthread_cond_t            CONDITION_VARIABLE

typedef struct pthread_t {
    void *handle;
    void *(*func)(void* arg);
    void *arg;
    void *ret;
} pthread_t;

typedef struct pthread_attr_t {
    void *unused;
} pthread_attr_t;

int pthread_create(pthread_t *thread, const pthread_attr_t *attr, 
                   void *(*start_routine)(void*), void *arg);
int pthread_join(pthread_t thread, void **retval);
int pthread_mutex_init(pthread_mutex_t *m, void* attr);
int pthread_mutex_destroy(pthread_mutex_t *m);
int pthread_mutex_lock(pthread_mutex_t *m);
int pthread_mutex_unlock(pthread_mutex_t *m);
int pthread_once(pthread_once_t *once_control, void (*init_routine)(void));
int pthread_cond_init(pthread_cond_t *cond, const void *unused_attr);
int pthread_cond_destroy(pthread_cond_t *cond);
int pthread_cond_broadcast(pthread_cond_t *cond);
int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex);
int pthread_cond_signal(pthread_cond_t *cond);

#define getpid                    GetCurrentProcessId
#define gettid                    GetCurrentThreadId
int get_proc_name(char *name, size_t len);


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

struct timezone
{
    int tz_minuteswest;
    int tz_dsttime;
};

int gettimeofday(struct timeval *tv, struct timezone *tz);
#define localtime_r(timep,result) localtime_s(result, timep)

#ifdef __cplusplus
}
#endif
#endif
