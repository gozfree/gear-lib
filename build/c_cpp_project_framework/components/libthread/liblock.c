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
#include "libthread.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined (OS_LINUX) || defined (OS_APPLE)
#include <unistd.h>
#include <sched.h>
#include <pthread.h>
#endif
#include <errno.h>

/******************************************************************************
 * spin lock APIs
 *****************************************************************************/
#if ( __i386__ || __i386 || __amd64__ || __amd64 )
#define cpu_pause() __asm__ ("pause")
#else
#define cpu_pause()
#endif

#if defined (__linux__) || defined (__CYGWIN__)
#define atomic_cmp_set(lock, old, set) \
    __sync_bool_compare_and_swap(lock, old, set)
#endif

int spin_lock(spin_lock_t *lock)
{
#if defined (__linux__) || defined (__CYGWIN__)
    int spin = 2048;
    int value = 1;
    int i, n;
    long g_ncpu = sysconf(_SC_NPROCESSORS_ONLN);
    for ( ;; ) {
        if (*lock == 0 && atomic_cmp_set(lock, 0, value)) {
            return 0;
        }
        if (g_ncpu > 1) {
            for (n = 1; n < spin; n <<= 1) {
                for (i = 0; i < n; i++) {
                    cpu_pause();
                }
                if (*lock == 0 && atomic_cmp_set(lock, 0, value)) {
                    return 0;
                }
            }
        }
        sched_yield();
    }
#endif
    return 0;
}

int spin_unlock(spin_lock_t *lock)
{
    *(lock) = 0;
    return 0;
}

int spin_trylock(spin_lock_t *lock)
{
#if defined (__linux__) || defined (__CYGWIN__)
    return (*(lock) == 0 && atomic_cmp_set(lock, 0, 1));
#else
    return 0;
#endif
}

/******************************************************************************
 * mutex lock APIs
 *****************************************************************************/
int mutex_lock_init(mutex_lock_t *lock)
{
#if defined (__linux__) || defined (__CYGWIN__)
    pthread_mutexattr_t  attr;

    if (0 != pthread_mutexattr_init(&attr)) {
        printf("pthread_mutexattr_init failed\n");
        return -1;
    }
    if (0 != pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK)) {
        printf("pthread_mutexattr_settype (PTHREAD_MUTEX_ERRORCHECK) failed\n");
        return -1;
    }
#endif
    pthread_mutex_init(lock, NULL);

#if defined (__linux__) || defined (__CYGWIN__)
    if (0 != pthread_mutexattr_destroy(&attr)) {
        printf("pthread_mutexattr_destroy failed\n");
    }
#endif
    return 0;
}

void mutex_lock_deinit(mutex_lock_t *ptr)
{
    int ret;
    pthread_mutex_t *lock = (pthread_mutex_t *)ptr;
    if (!ptr) {
        return;
    }
    ret = pthread_mutex_destroy(lock);
    if (ret != 0) {
        switch (ret) {
        case EBUSY:
            printf("the mutex is currently locked.\n");
            break;
        default:
            printf("pthread_mutex_trylock error:%s.\n", strerror(ret));
            break;
        }
    }
}

int mutex_trylock(mutex_lock_t *ptr)
{
    int ret = 0;
#if defined (__linux__) || defined (__CYGWIN__)
    pthread_mutex_t *lock = (pthread_mutex_t *)ptr;
    if (!ptr) {
        return -1;
    }
    ret = pthread_mutex_trylock(lock);
    if (ret != 0) {
        switch (ret) {
        case EBUSY:
            printf("the mutex could not be acquired"
                   " because it was currently locked.\n");
            break;
        case EINVAL:
            printf("the mutex has not been properly initialized.\n");
            break;
        default:
            printf("pthread_mutex_trylock error:%s.\n", strerror(ret));
            break;
        }
    }
#endif
    return ret;
}

int mutex_lock(mutex_lock_t *ptr)
{
    int ret;
    pthread_mutex_t *lock = (pthread_mutex_t *)ptr;
    if (!ptr) {
        return -1;
    }
    ret = pthread_mutex_lock(lock);
    if (ret != 0) {
        switch (ret) {
        case EDEADLK:
            printf("the mutex is already locked by the calling thread"
                   " (``error checking'' mutexes only).\n");
            break;
        case EINVAL:
            printf("the mutex has not been properly initialized.\n");
            break;
        default:
            printf("pthread_mutex_trylock error:%s.\n", strerror(ret));
            break;
        }
    }
    return ret;
}

int mutex_unlock(mutex_lock_t *ptr)
{
    int ret;
    pthread_mutex_t *lock = (pthread_mutex_t *)ptr;
    if (!ptr) {
        return -1;
    }
    ret = pthread_mutex_unlock(lock);
    if (ret != 0) {
        switch (ret) {
        case EPERM:
            printf("the calling thread does not own the mutex"
                   " (``error checking'' mutexes only).\n");
            break;
        case EINVAL:
            printf("the mutex has not been properly initialized.\n");
            break;
        default:
            printf("pthread_mutex_trylock error:%s.\n", strerror(ret));
            break;
        }
    }
    return ret;
}

int mutex_cond_init(mutex_cond_t *cond)
{
    if (!cond) {
        printf("malloc pthread_cond_t failed:%d\n", errno);
        return -1;
    }
    //never return an error code
    pthread_cond_init(cond, NULL);
    return 0;
}

void mutex_cond_deinit(mutex_cond_t *ptr)
{
    int ret;
    pthread_cond_t *cond = (pthread_cond_t *)ptr;
    if (!ptr) {
        return;
    }
    ret = pthread_cond_destroy(cond);
    if (ret != 0) {
        switch (ret) {
        case EBUSY:
            printf("some threads are currently waiting on cond.\n");
            break;
        default:
            printf("pthread_cond_destroy error:%s.\n", strerror(ret));
            break;
        }
    }
}

int mutex_cond_wait(mutex_lock_t *mutexp, mutex_cond_t *condp, int64_t ms)
{
    int ret = 0;
#if defined (__linux__) || defined (__CYGWIN__)
    int retry = 3;
    struct timespec ts;
    pthread_mutex_t *mutex = (pthread_mutex_t *)mutexp;
    pthread_cond_t *cond = (pthread_cond_t *)condp;
    if (!condp || !mutexp) {
        return -1;
    }
    if (ms <= 0) {
        //never return an error code
        pthread_cond_wait(cond, mutex);
    } else {
        clock_gettime(CLOCK_REALTIME, &ts);
        uint64_t ns = ts.tv_sec * 1000 * 1000 * 1000 + ts.tv_nsec;
        ns += ms * 1000 * 1000;
        ts.tv_sec = ns / (1000 * 1000 * 1000);
        ts.tv_nsec = ns % 1000 * 1000 * 1000;
wait:
        ret = pthread_cond_timedwait(cond, mutex, &ts);
        if (ret != 0) {
            switch (ret) {
            case ETIMEDOUT:
                printf("the condition variable was not signaled "
                       "until the timeout specified by abstime.\n");
                break;
            case EINTR:
                printf("pthread_cond_timedwait was interrupted by a signal.\n");
                if (--retry != 0) {
                    goto wait;
                }
                break;
            default:
                printf("pthread_cond_timedwait error:%s.\n", strerror(ret));
                break;
            }
        }
    }
#endif
    return ret;
}

void mutex_cond_signal(mutex_cond_t *ptr)
{
    pthread_cond_t *cond = (pthread_cond_t *)ptr;
    if (!ptr) {
        return;
    }
    //never return an error code
    pthread_cond_signal(cond);
}

void mutex_cond_signal_all(mutex_cond_t *ptr)
{
    pthread_cond_t *cond = (pthread_cond_t *)ptr;
    if (!ptr) {
        return;
    }
    //never return an error code
    pthread_cond_broadcast(cond);
}

/******************************************************************************
 * read-write lock APIs
 *****************************************************************************/
#if defined(_POSIX_READER_WRITER_LOCKS)
int rwlock_init(rw_lock_t *lock)
{
    int ret;
    if (!lock) {
        printf("malloc pthread_rwlock_t failed:%d\n", errno);
        return -1;
    }
    ret = pthread_rwlock_init(lock, NULL);
    if (ret != 0) {
        switch (ret) {
        case EAGAIN:
            printf("The system lacked the necessary resources (other than"
                   " memory) to initialize another read-write lock.\n");
            break;
        case ENOMEM:
            printf("Insufficient memory exists to initialize the "
                   "read-write lock.\n");
            break;
        case EPERM:
            printf("The caller does not have the privilege to perform "
                   "the operation.\n");
            break;
        default:
            printf("pthread_rwlock_init failed:%d\n", ret);
            break;
        }
        lock = NULL;
    }

    return 0;
}

void rwlock_deinit(rw_lock_t *ptr)
{
    pthread_rwlock_t *lock = (pthread_rwlock_t *)ptr;
    if (!ptr) {
        return;
    }
    if (0 != pthread_rwlock_destroy(lock)) {
        printf("pthread_rwlock_destroy failed!\n");
    }
}

int rwlock_rdlock(rw_lock_t *ptr)
{
    int ret;
    pthread_rwlock_t *lock = (pthread_rwlock_t *)ptr;
    if (!ptr) {
        return -1;
    }
    ret = pthread_rwlock_rdlock(lock);
    if (ret != 0) {
        switch (ret) {
        case EBUSY:
            printf("The read-write lock could not be acquired for reading "
                   "because a writer holds the lock or a writer with the "
                   "appropriate priority was blocked on it.\n");
            break;
        case EAGAIN:
            printf("The read lock could not be acquired because the maximum "
                   "number of read locks for rwlock has been exceeded.\n");
            break;
        case EDEADLK:
            printf("A deadlock condition was detected or the current thread "
                   "already owns the read-write lock for writing.\n");
            break;
        default:
            printf("pthread_rwlock_rdlock failed!\n");
            break;
        }
    }
    return ret;
}

int rwlock_tryrdlock(rw_lock_t *ptr)
{
    int ret = 0;
#if defined (__linux__) || defined (__CYGWIN__)
    pthread_rwlock_t *lock = (pthread_rwlock_t *)ptr;
    if (!ptr) {
        return -1;
    }
    ret = pthread_rwlock_tryrdlock(lock);
    if (ret != 0) {
        switch (ret) {
        case EBUSY:
            printf("The read-write lock could not be acquired for reading "
                   "because a writer holds the lock or a writer with the "
                   "appropriate priority was blocked on it.\n");
            break;
        case EAGAIN:
            printf("The read lock could not be acquired because the maximum "
                   "number of read locks for rwlock has been exceeded.\n");
            break;
        case EDEADLK:
            printf("A deadlock condition was detected or the current thread "
                   "already owns the read-write lock for writing.\n");
            break;
        default:
            printf("pthread_rwlock_tryrdlock failed!\n");
            break;
        }
    }
#endif
    return ret;
}

int rwlock_wrlock(rw_lock_t *ptr)
{
    int ret = 0;
#if defined (__linux__) || defined (__CYGWIN__)
    pthread_rwlock_t *lock = (pthread_rwlock_t *)ptr;
    if (!ptr) {
        return -1;
    }
    ret = pthread_rwlock_wrlock(lock);
    if (ret != 0) {
        switch (ret) {
        case EDEADLK:
            printf("A deadlock condition was detected or the current thread "
                  "already owns the read-write lock for writing or reading.\n");
            break;
        default:
            printf("pthread_rwlock_wrlock failed!\n");
            break;
        }
    }
#endif
    return ret;
}

int rwlock_trywrlock(rw_lock_t *ptr)
{
    int ret = 0;
#if defined (__linux__) || defined (__CYGWIN__)
    pthread_rwlock_t *lock = (pthread_rwlock_t *)ptr;
    if (!ptr) {
        return -1;
    }
    ret = pthread_rwlock_trywrlock(lock);
    if (ret != 0) {
        switch (ret) {
        case EBUSY:
            printf("The read-write lock could not be acquired for writing "
                   "because it was already locked for reading or writing.\n");
            break;

        default:
            printf("pthread_rwlock_trywrlock failed!\n");
            break;
        }
    }
#endif
    return ret;
}

int rwlock_unlock(rw_lock_t *ptr)
{
    int ret;
    pthread_rwlock_t *lock = (pthread_rwlock_t *)ptr;
    if (!ptr) {
        return -1;
    }
    ret = pthread_rwlock_unlock(lock);
    if (ret != 0) {
        switch (ret) {
        default:
            printf("pthread_rwlock_unlock failed!\n");
            break;
        }
    }
    return ret;
}
#endif

/******************************************************************************
 * sem lock APIs
 *****************************************************************************/
int sem_lock_init(sem_lock_t *lock)
{
#if defined (OS_LINUX) || defined (OS_WINDOWS)
    int pshared = 0;//0: threads, 1: processes
    if (0 != sem_init(lock, pshared, 0)) {
        printf("sem_init failed %d:%s\n", errno, strerror(errno));
        return -1;
    }
#endif
    return 0;
}

void sem_lock_deinit(sem_lock_t *ptr)
{
#if defined (OS_LINUX) || defined (OS_WINDOWS)
    sem_t *lock = (sem_t *)ptr;
    if (!ptr) {
        return;
    }
    if (0 != sem_destroy(lock)) {
        printf("sem_destroy %d:%s\n", errno , strerror(errno));
    }
#endif
}

int sem_lock_wait(sem_lock_t *ptr, int64_t ms)
{
    int ret = 0;
#if defined (OS_LINUX)
    struct timespec ts;
    sem_t *lock = (sem_t *)ptr;
    if (!ptr) {
        return -1;
    }
    if (ms < 0) {
        ret = sem_wait(lock);
        if (ret != 0) {
            switch (errno) {
            case EINTR:
                printf("The call was interrupted by a signal handler.\n");
                break;
            case EINVAL:
                printf("sem is not a valid semaphore.\n");
                break;
            }
        }
    } else {
        clock_gettime(CLOCK_REALTIME, &ts);
        uint64_t ns = ts.tv_sec * 1000 * 1000 * 1000 + ts.tv_nsec;
        ns += ms * 1000 * 1000;
        ts.tv_sec = ns / (1000 * 1000 * 1000);
        ts.tv_nsec = ns % 1000 * 1000 * 1000;
        ret = sem_timedwait(lock, &ts);
        if (ret != 0) {
            switch (errno) {
            case EINVAL:
                printf("The value of abs_timeout.tv_nsecs is less than 0, "
                       "or greater than or equal to 1000 million.\n");
                break;
            case ETIMEDOUT:
                printf("The call timed out before the semaphore could be locked.\n");
                break;
            }
        }
    }
#endif
    return ret;
}

int sem_lock_trywait(sem_lock_t *ptr)
{
    int ret = 0;
#if defined (__linux__) || defined (__CYGWIN__)
    sem_t *lock = (sem_t *)ptr;
    if (!ptr) {
        return -1;
    }
    ret = sem_trywait(lock);
    if (ret != 0) {
        switch (errno) {
        case EAGAIN:
            printf("The operation could not be performed without blocking\n");
            break;
        }
    }
#endif
    return ret;
}

int sem_lock_signal(sem_lock_t *ptr)
{
    int ret = 0;
#ifndef FREERTOS
    sem_t *lock = (sem_t *)ptr;
    if (!ptr) {
        return -1;
    }
    ret = sem_post(lock);
    if (ret != 0) {
        switch (errno) {
        case EINVAL:
            printf("sem is not a valid semaphore.\n");
            break;
        case EOVERFLOW:
            printf("The maximum allowable value for a semaphore would be exceeded.\n");
            break;
        }
    }
#endif
    return ret;
}
