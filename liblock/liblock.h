/******************************************************************************
 * Copyright (C) 2014-2018 Zhifeng Gong <gozfree@163.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with libraries; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 ******************************************************************************/
#ifndef LIBLOCK_H
#define LIBLOCK_H

#include <stdint.h>
#include <pthread.h>
#include <semaphore.h>

#ifdef __cplusplus
extern "C" {
#endif

enum lock_type {
    LOCK_SPIN = 0,
    LOCK_MUTEX,
    LOCK_COND,
    LOCK_RW,
    LOCK_SEM,
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
typedef pthread_rwlock_t rw_lock_t;
int rwlock_init(rw_lock_t *lock);
int rwlock_rdlock(rw_lock_t *lock);
int rwlock_tryrdlock(rw_lock_t *lock);
int rwlock_wrlock(rw_lock_t *lock);
int rwlock_trywrlock(rw_lock_t *lock);
int rwlock_unlock(rw_lock_t *lock);
void rwlock_deinit(rw_lock_t *lock);


/*
 * sem lock implemented by Unnamed semaphores (memory-based semaphores) APIs
 */
typedef sem_t sem_lock_t;
int sem_lock_init(sem_lock_t *lock);
int sem_lock_wait(sem_lock_t *lock, int64_t ms);
int sem_lock_trywait(sem_lock_t *lock);
int sem_lock_signal(sem_lock_t *lock);
void sem_lock_deinit(sem_lock_t *lock);


#ifdef __cplusplus
}
#endif
#endif
