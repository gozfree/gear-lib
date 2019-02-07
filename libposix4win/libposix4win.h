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
#include <direct.h>
#include <winsock2.h>
#include <windows.h>
#include <process.h>


#ifdef __cplusplus
extern "C" {
#endif


#define bool                int
#define false               0
#define true                1

#define inline              __inline
#define ssize_t             SSIZE_T
#define __func__            __FUNCTION__
#define off_t               SSIZE_T

#define PATH_SPLIT                '\\'
#define getcwd(buf, size)         GetModuleFileName(NULL, buf, size)
#define mkdir(path,mode)          _mkdir(path)
#define localtime_r(timep,result) localtime_s(result, timep)
#define _gettid(void)             GetCurrentProcessId()
#define iovec                     _WSABUF
#define iov_len                   len
#define iov_base                  buf

struct stat {
#if 0
    dev_t         st_dev;
    ino_t         st_ino;
    mode_t        st_mode;
    nlink_t       st_nlink;
    uid_t         st_uid;
    gid_t         st_gid;
    dev_t         st_rdev;
#endif
    off_t         st_size;
#if 0
    unsigned long st_blksize;
    unsigned long st_blocks;
    time_t        st_atime;
#endif
};

int stat(const char *file, struct stat *buf);


/******************************************************************************/

#define pthread_once_t INIT_ONCE
#define pthread_mutex_t SRWLOCK
#define pthread_cond_t CONDITION_VARIABLE

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

/******************************************************************************/
#define dlopen(name, flags) win32_dlopen(name)
#define dlclose FreeLibrary
#define dlsym GetProcAddress

#ifdef __cplusplus
}
#endif
#endif
