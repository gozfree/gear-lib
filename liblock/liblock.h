/******************************************************************************
 * Copyright (C) 2014-2015
 * file:    liblock.h
 * author:  gozfree <gozfree@163.com>
 * created: 2016-06-22 14:09:11
 * updated: 2016-06-22 14:09:11
 *****************************************************************************/
#ifndef LIBLOCK_H
#define LIBLOCK_H

#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif

/*
 * spin lock implemented by atomic APIs
 */
typedef int spin_lock_t;
spin_lock_t *spin_lock_init();
int spin_lock(spin_lock_t *lock);
int spin_unlock(spin_lock_t *lock);
int spin_trylock(spin_lock_t *lock);
void spin_lock_deinit(spin_lock_t *lock);


/*
 * mutex lock implemented by pthread_mutex APIs
 */
typedef void mutex_lock_t;
mutex_lock_t *mutex_lock_init();
int mutex_trylock(mutex_lock_t *lock);
int mutex_lock(mutex_lock_t *lock);
int mutex_unlock(mutex_lock_t *lock);
void mutex_lock_deinit(mutex_lock_t *lock);


/*
 * external APIs of mutex condition
 */
typedef void mutex_cond_t;
mutex_cond_t *mutex_cond_init();
int mutex_cond_wait(mutex_lock_t *mutex, mutex_cond_t *cond, int64_t ms);
void mutex_cond_signal(mutex_cond_t *cond);
void mutex_cond_signal_all(mutex_cond_t *cond);
void mutex_cond_deinit(mutex_cond_t *cond);


/*
 * read-write lock implemented by pthread_rwlock APIs
 */
typedef void rw_lock_t;
rw_lock_t *rwlock_init();
int rwlock_rdlock(rw_lock_t *lock);
int rwlock_tryrdlock(rw_lock_t *lock);
int rwlock_wrlock(rw_lock_t *lock);
int rwlock_trywrlock(rw_lock_t *lock);
int rwlock_unlock(rw_lock_t *lock);
void rwlock_deinit(rw_lock_t *lock);


/*
 * sem lock implemented by Unnamed semaphores (memory-based semaphores) APIs
 */
typedef void sem_lock_t;
sem_lock_t *sem_lock_init();
int sem_lock_wait(sem_lock_t *lock, int64_t ms);
int sem_lock_trywait(sem_lock_t *lock);
int sem_lock_signal(sem_lock_t *lock);
void sem_lock_deinit(sem_lock_t *lock);


#ifdef __cplusplus
}
#endif
#endif
