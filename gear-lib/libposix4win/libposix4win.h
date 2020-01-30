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
#include <ws2tcpip.h>
#include <process.h>
#include <tlhelp32.h>
#include <fcntl.h>
#include <io.h>
#include <sys/types.h>
#include <sys/stat.h>

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

char *dup_wchar_to_utf8(wchar_t *w);

#define memalign(align, size)     _aligned_malloc(size, align)


/******************************************************************************
 * sys file APIs
 ******************************************************************************/
#define STDIN_FILENO              0       /* standard input file descriptor */
#define STDOUT_FILENO             1       /* standard output file descriptor */
#define STDERR_FILENO             2       /* standard error file descriptor */
#define MAXPATHLEN                1024
#define PATH_MAX                  4096

typedef int                       mode_t;

#define F_OK                      0
#define R_OK                      4
#define W_OK                      2
#define X_OK                      1

int stat(const char *file, struct stat *buf);
int access(const char *pathname, int mode);
#define mkdir(path,mode)          _mkdir(path)
#define getcwd(buf, size)         GetModuleFileName(NULL, buf, size)


/******************************************************************************
 * pthread APIs
 ******************************************************************************/

#define pthread_once_t            INIT_ONCE
#define PTHREAD_ONCE_INIT         INIT_ONCE_STATIC_INIT
#define pthread_mutex_t           HANDLE
#define pthread_cond_t            CONDITION_VARIABLE
#define pthread_rwlock_t          SRWLOCK
#define sem_t                     HANDLE

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

int pthread_attr_init(pthread_attr_t *attr);
void pthread_attr_destroy(pthread_attr_t *attr);
int pthread_mutex_init(pthread_mutex_t *m, void *attr);
int pthread_mutex_destroy(pthread_mutex_t *m);
int pthread_mutex_lock(pthread_mutex_t *m);
int pthread_mutex_unlock(pthread_mutex_t *m);

int pthread_rwlock_init(pthread_rwlock_t *m, void *attr);
int pthread_rwlock_destroy(pthread_rwlock_t *m);
int pthread_rwlock_rdlock(pthread_rwlock_t *m);
int pthread_rwlock_wrlock(pthread_rwlock_t *m);
int pthread_rwlock_unlock(pthread_rwlock_t *m);

int sem_init(sem_t *sem, int pshared, unsigned int value);
int sem_destroy(sem_t *sem);
int sem_wait(sem_t *sem);
int sem_trywait(sem_t *sem);
int sem_timedwait(sem_t *sem, const struct timespec *abstime);
int sem_post(sem_t *sem);




int pthread_once(pthread_once_t *once_control, void (*init_routine)(void));
int pthread_cond_init(pthread_cond_t *cond, const void *unused_attr);
int pthread_cond_destroy(pthread_cond_t *cond);
int pthread_cond_broadcast(pthread_cond_t *cond);
int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex);
int pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex, int ms);
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
#define sleep(n) Sleep(n*1000)

typedef int clockid_t;
#define CLOCK_REALTIME           ((clockid_t)1)
#define CLOCK_PROCESS_CPUTIME_ID ((clockid_t)2)
#define CLOCK_THREAD_CPUTIME_ID  ((clockid_t)3)
#define CLOCK_MONOTONIC          ((clockid_t)4)


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

int get_nprocs();


#ifdef __cplusplus
}
#endif
#endif
