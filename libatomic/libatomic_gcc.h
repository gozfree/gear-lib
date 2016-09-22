/******************************************************************************
 * Copyright (C) 2014-2015
 * file:    libatomic_gcc.h
 * author:  gozfree <gozfree@163.com>
 * created: 2016-04-24 17:57
 * updated: 2016-04-24 17:57
 ******************************************************************************/
#ifndef LIBATOMIC_GCC_H
#define LIBATOMIC_GCC_H

#include <stdint.h>
#include "libatomic.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GCC_VERSION (__GNUC__*100 + __GNUC_MINOR__*10 + __GNUC_PATCHLEVEL__)

#if GCC_VERSION > 463
#define HAVE_ATOMIC_COMPARE_EXCHANGE 1
#else
#define HAVE_ATOMIC_COMPARE_EXCHANGE 0
#endif

#define HAVE_SYNC_VAL_COMPARE_AND_SWAP 1

#define atomic_int_get atomic_int_get_gcc
static inline int atomic_int_get_gcc(volatile int *ptr)
{
#if HAVE_ATOMIC_COMPARE_EXCHANGE
    return __atomic_load_n(ptr, __ATOMIC_SEQ_CST);
#else
    __sync_synchronize();
    return *ptr;
#endif
}

#define atomic_int_set atomic_int_set_gcc
static inline void atomic_int_set_gcc(volatile int *ptr, int val)
{
#if HAVE_ATOMIC_COMPARE_EXCHANGE
    __atomic_store_n(ptr, val, __ATOMIC_SEQ_CST);
#else
    *ptr = val;
    __sync_synchronize();
#endif
}

#define atomic_int_add_and_fetch atomic_int_add_and_fetch_gcc
static inline int atomic_int_add_and_fetch_gcc(volatile int *ptr, int inc)
{
#if HAVE_ATOMIC_COMPARE_EXCHANGE
    return __atomic_add_fetch(ptr, inc, __ATOMIC_SEQ_CST);
#else
    return __sync_add_and_fetch(ptr, inc);
#endif
}

#define atomic_int_sub_and_fetch atomic_int_sub_and_fetch_gcc
static inline int atomic_int_sub_and_fetch_gcc(volatile int *ptr, int inc)
{
#if HAVE_ATOMIC_COMPARE_EXCHANGE
    return __atomic_sub_fetch(ptr, inc, __ATOMIC_SEQ_CST);
#else
    return __sync_sub_and_fetch(ptr, inc);
#endif
}

#define atomic_int_inc atomic_int_inc_gcc
static inline int atomic_int_inc_gcc(volatile int *ptr)
{
    return atomic_int_add_and_fetch(ptr, 1);
}

#define atomic_int_dec atomic_int_dec_gcc
static inline int atomic_int_dec_gcc(volatile int *ptr)
{
    return atomic_int_add_and_fetch(ptr, -1);
}



#define atomic_ptr_cas atomic_ptr_cas_gcc
static inline void *atomic_ptr_cas_gcc(void * volatile *ptr,
                                       void *oldval, void *newval)
{
#if HAVE_SYNC_VAL_COMPARE_AND_SWAP
#ifdef __ARMCC_VERSION
    // armcc will throw an error if ptr is not an integer type
    volatile uintptr_t *tmp = (volatile uintptr_t*)ptr;
    return (void*)__sync_val_compare_and_swap(tmp, oldval, newval);
#else
    return __sync_val_compare_and_swap(ptr, oldval, newval);
#endif
#else
    __atomic_compare_exchange_n(ptr, &oldval, newval, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
    return oldval;
#endif
}


#ifdef __cplusplus
}
#endif
#endif
