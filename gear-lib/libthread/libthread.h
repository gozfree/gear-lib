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
#ifndef LIBTHREAD_H
#define LIBTHREAD_H

#include <libposix.h>
#include <stdio.h>
#include <stdint.h>
#if defined (OS_LINUX) || defined (OS_APPLE)
#include <stdbool.h>
#define _GNU_SOURCE
#include <pthread.h>
#include <semaphore.h>
#define _POSIX_RW_LOCKS
#elif defined (OS_WINDOWS)
#define _POSIX_RW_LOCKS
#elif defined (OS_RTTHREAD)
#include <pthread.h>
#include <semaphore.h>
#endif

#define LIBTHREAD_VERSION "0.1.2"

#ifdef __cplusplus
extern "C" {
#endif

enum lock_type {
    THREAD_LOCK_SPIN = 0,
    THREAD_LOCK_MUTEX,
    THREAD_LOCK_COND,
    THREAD_LOCK_RW,
    THREAD_LOCK_SEM,
};

/*
 * spin lock implemented by atomic APIs
 */
typedef int spin_lock_t;
int spin_lock(spin_lock_t *lock);
int spin_unlock(spin_lock_t *lock);
int spin_trylock(spin_lock_t *lock);

/*
 * mutex lock implemented by pthread_mutex APIs
 */
typedef pthread_mutex_t mutex_lock_t;
int mutex_lock_init(mutex_lock_t *lock);
int mutex_trylock(mutex_lock_t *lock);
int mutex_lock(mutex_lock_t *lock);
int mutex_unlock(mutex_lock_t *lock);
void mutex_lock_deinit(mutex_lock_t *lock);


/*
 * external APIs of mutex condition
 */
typedef pthread_cond_t mutex_cond_t;
int mutex_cond_init(mutex_cond_t *cond);
int mutex_cond_wait(mutex_lock_t *mutex, mutex_cond_t *cond, int64_t ms);
void mutex_cond_signal(mutex_cond_t *cond);
void mutex_cond_signal_all(mutex_cond_t *cond);
void mutex_cond_deinit(mutex_cond_t *cond);


/*
 * read-write lock implemented by pthread_rwlock APIs
 */
#if defined(_POSIX_RW_LOCKS)
typedef pthread_rwlock_t rw_lock_t;
int rwlock_init(rw_lock_t *lock);
int rwlock_rdlock(rw_lock_t *lock);
int rwlock_tryrdlock(rw_lock_t *lock);
int rwlock_wrlock(rw_lock_t *lock);
int rwlock_trywrlock(rw_lock_t *lock);
int rwlock_unlock(rw_lock_t *lock);
void rwlock_deinit(rw_lock_t *lock);
#endif


/*
 * sem lock implemented by Unnamed semaphores (memory-based semaphores) APIs
 */
#if defined (FREERTOS)
typedef void* sem_t;
#endif
#define sem_lock_t sem_t
int sem_lock_init(sem_lock_t *lock);
int sem_lock_wait(sem_lock_t *lock, int64_t ms);
int sem_lock_trywait(sem_lock_t *lock);
int sem_lock_signal(sem_lock_t *lock);
void sem_lock_deinit(sem_lock_t *lock);

#define THREAD_NAME_LEN 16

typedef struct thread {
    pthread_t tid;
    pthread_attr_t attr;
    char name[THREAD_NAME_LEN];
    enum lock_type type;
    union {
        spin_lock_t spin;
        mutex_lock_t mutex;
        sem_lock_t sem;
    } lock;
    mutex_cond_t cond;
    bool run;
    void *(*func)(struct thread *, void *);
    void *arg;
} thread_t;

GEAR_API struct thread *thread_create(void *(*func)(struct thread *, void *), void *arg);
GEAR_API int thread_join(struct thread *t);
GEAR_API void thread_destroy(struct thread *t);
GEAR_API void thread_get_info(struct thread *t);
GEAR_API int thread_set_name(struct thread *t, const char *name);

GEAR_API int thread_lock(struct thread *t);
GEAR_API int thread_unlock(struct thread *t);
GEAR_API int thread_wait(struct thread *t, int64_t ms);
GEAR_API int thread_signal(struct thread *t);
GEAR_API int thread_signal_all(struct thread *t);

#ifdef __cplusplus
}
#endif
#endif
