/******************************************************************************
 * Copyright (C) 2014-2015
 * file:    liblock.h
 * author:  gozfree <gozfree@163.com>
 * created: 2016-06-22 14:09:11
 * updated: 2016-06-22 14:09:11
 *****************************************************************************/
#ifndef _LIBLOCK_H_
#define _LIBLOCK_H_



#ifdef __cplusplus
extern "C" {
#endif

/*
 * spin lock implemented by atomic APIs
 */
typedef int spin_lock_t;
spin_lock_t *spin_init();
int spin_lock(spin_lock_t *lock);
int spin_unlock(spin_lock_t *lock);
int spin_trylock(spin_lock_t *lock);
void spin_deinit(spin_lock_t *lock);



/*
 * mutex lock implemented by pthread_mutex APIs
 */
typedef void mutex_lock_t;
mutex_lock_t *mutex_init();
void mutex_deinit(mutex_lock_t *lock);
int mutex_trylock(mutex_lock_t *lock);
int mutex_lock(mutex_lock_t *lock);
int mutex_unlock(mutex_lock_t *lock);



#ifdef __cplusplus
}
#endif
#endif
