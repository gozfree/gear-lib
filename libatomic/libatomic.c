/******************************************************************************
 * Copyright (C) 2014-2015
 * file:    libatomic.c
 * author:  gozfree <gozfree@163.com>
 * created: 2016-04-24 17:25
 * updated: 2016-04-24 17:25
 ******************************************************************************/
#include "libatomic.h"

#define HAVE_PTHREADS   1

#if !HAVE_ATOMICS_NATIVE

#if HAVE_PTHREADS

#include <pthread.h>

static pthread_mutex_t atomic_lock = PTHREAD_MUTEX_INITIALIZER;

int atomic_int_get(volatile int *ptr)
{
    int res;

    pthread_mutex_lock(&atomic_lock);
    res = *ptr;
    pthread_mutex_unlock(&atomic_lock);

    return res;
}

void atomic_int_set(volatile int *ptr, int val)
{
    pthread_mutex_lock(&atomic_lock);
    *ptr = val;
    pthread_mutex_unlock(&atomic_lock);
}

int atomic_int_add_and_fetch(volatile int *ptr, int inc)
{
    int res;

    pthread_mutex_lock(&atomic_lock);
    *ptr += inc;
    res = *ptr;
    pthread_mutex_unlock(&atomic_lock);

    return res;
}

void *atomic_ptr_cas(void * volatile *ptr, void *oldval, void *newval)
{
    void *ret;
    pthread_mutex_lock(&atomic_lock);
    ret = *ptr;
    if (ret == oldval)
        *ptr = newval;
    pthread_mutex_unlock(&atomic_lock);
    return ret;
}



#elif !HAVE_THREADS

int atomic_int_get(volatile int *ptr)
{
    return *ptr;
}

void atomic_int_set(volatile int *ptr, int val)
{
    *ptr = val;
}

int atomic_int_add_and_fetch(volatile int *ptr, int inc)
{
    *ptr += inc;
    return *ptr;
}

void *atomic_ptr_cas(void * volatile *ptr, void *oldval, void *newval)
{
    if (*ptr == oldval) {
        *ptr = newval;
        return oldval;
    }
    return *ptr;
}

#else /* HAVE_THREADS */

/* This should never trigger, unless a new threading implementation
 * without correct atomics dependencies in configure or a corresponding
 * atomics implementation is added. */
#error "Threading is enabled, but there is no implementation of atomic operations available"

#endif /* HAVE_PTHREADS */

int atomic_int_inc(volatile int *ptr)
{
    return atomic_int_add_and_fetch(ptr, 1);
}

int atomic_int_dec(volatile int *ptr)
{
    return atomic_int_add_and_fetch(ptr, -1);
}
#endif /* !HAVE_ATOMICS_NATIVE */


