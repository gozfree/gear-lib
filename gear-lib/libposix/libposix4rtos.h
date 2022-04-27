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
#ifndef LIBPOSIX4RTOS_H
#define LIBPOSIX4RTOS_H

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#ifdef ESP32
#include <pthread.h>
#else
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#endif
#include "kernel_list.h"

#define GEAR_API

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * basic types
 ******************************************************************************/
//typedef int                       bool;

struct iovec {
    void *iov_base;
    size_t iov_len;
};

/*
 * below defined iovec to solve redefinition of 'struct iovec' of lwip
 */
#define iovec iovec
//ssize_t writev(int fd, const struct iovec *iov, int iovcnt);

#ifdef ESP32
#include <sys/uio.h>
#endif

GEAR_API int eventfd(unsigned int initval, int flags);
GEAR_API void eventfd_close(int fd);


/******************************************************************************
 * I/O string APIs
 ******************************************************************************/
#define PRId8                     "hhd"
#define PRId16                    "hd"
#define PRId32                    "ld"
#define PRId64                    "lld"
#define PRIu8                     "hhu"
#define PRIu16                    "hu"
#define PRIu32                    "lu"
#define PRIu64                    "llu"

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



#ifndef ESP32

/******************************************************************************
 * time APIs
 ******************************************************************************/
typedef unsigned long useconds_t;
typedef long time_t;
struct timespec {
    time_t tv_sec;
    long tv_nsec;
};

int usleep(useconds_t us);
unsigned int sleep(unsigned int seconds);


/******************************************************************************
 * pthread APIs
 ******************************************************************************/
#ifndef pthread_mutex_t
typedef SemaphoreHandle_t  pthread_mutex_t;
#endif

typedef struct pthread_t {
    void *handle;
    void *(*func)(void* arg);
    void *arg;
    void *ret;
} pthread_t;

typedef struct pthread_attr_t {
    int sched_priority;
    int stack_depth;
#define MAX_THREAD_NAME 64
    const char *thread_name[MAX_THREAD_NAME];
} pthread_attr_t;

typedef struct pthread_cond_t {
    EventGroupHandle_t event;
    EventBits_t evbits;
} pthread_cond_t;

int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                   void *(*start_routine)(void*), void *arg);
int pthread_join(pthread_t thread, void **retval);

int pthread_attr_init(pthread_attr_t *attr);
void pthread_attr_destroy(pthread_attr_t *attr);

int pthread_mutex_init(pthread_mutex_t *m, void *attr);
int pthread_mutex_destroy(pthread_mutex_t *m);
int pthread_mutex_lock(pthread_mutex_t *m);
int pthread_mutex_unlock(pthread_mutex_t *m);

int pthread_cond_init(pthread_cond_t *cond, const void *unused_attr);
int pthread_cond_destroy(pthread_cond_t *cond);
int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex);
int pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex, int ms);
int pthread_cond_signal(pthread_cond_t *cond);


/******************************************************************************
 * memory APIs
 ******************************************************************************/
#endif
void *rtos_aligned_alloc(size_t alignment, size_t size);
void rtos_aligned_free(void *ptr);

#define aligned_alloc(align, size)    rtos_aligned_alloc(align, size)
#define aligned_free(ptr)             rtos_aligned_free(ptr)
#ifdef __cplusplus
}
#endif
#endif
