/******************************************************************************
 * Copyright (C) 2014-2015
 * file:    liblock.c
 * author:  gozfree <gozfree@163.com>
 * created: 2016-06-22 14:09:11
 * updated: 2016-06-22 14:09:11
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <sched.h>

#include "liblock.h"

/******************************************************************************
 * spin lock APIs
 *****************************************************************************/
#define cpu_pause() __asm__ ("pause")
#define atomic_cmp_set(lock, old, set) \
    __sync_bool_compare_and_swap(lock, old, set)

static long g_ncpu = 1;

spin_lock_t *spin_init()
{
    spin_lock_t *lock = (spin_lock_t *)calloc(1, sizeof(spin_lock_t));
    if (!lock) {
        printf("malloc spin_lock_t failed:%d\n", errno);
        return NULL;
    }
    g_ncpu = sysconf(_SC_NPROCESSORS_ONLN);
    return lock;
}

int spin_lock(spin_lock_t *lock)
{
    int spin = 2048;
    int value = 1;
    int i, n;
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
    return 0;
}

int spin_unlock(spin_lock_t *lock)
{
    *(lock) = 0;
    return 0;
}

int spin_trylock(spin_lock_t *lock)
{
    return (*(lock) == 0 && atomic_cmp_set(lock, 0, 1));
}

void spin_deinit(spin_lock_t *lock)
{
    if (!lock) {
        return;
    }
    free(lock);
}

/******************************************************************************
 * mutex lock APIs
 *****************************************************************************/

mutex_lock_t *mutex_init()
{
    pthread_mutex_t *lock = (pthread_mutex_t *)calloc(1,
                                               sizeof(pthread_mutex_t));
    if (!lock) {
        printf("malloc pthread_mutex_t failed:%d\n", errno);
        return NULL;
    }
    pthread_mutex_init(lock, NULL);
    return lock;
}

void mutex_deinit(mutex_lock_t *lock)
{
    if (!lock) {
        return;
    }
    int ret = pthread_mutex_destroy(lock);
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
    free(lock);
}

int mutex_trylock(mutex_lock_t *lock)
{
    if (!lock) {
        return -1;
    }
    int ret = pthread_mutex_trylock(lock);
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
    return ret;
}

int mutex_lock(mutex_lock_t *lock)
{
    if (!lock) {
        return -1;
    }
    int ret = pthread_mutex_lock(lock);
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

int mutex_unlock(mutex_lock_t *lock)
{
    if (!lock) {
        return -1;
    }
    int ret = pthread_mutex_unlock(lock);
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
