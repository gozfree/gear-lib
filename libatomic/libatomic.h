/******************************************************************************
 * Copyright (C) 2014-2015
 * file:    libatomic.h
 * author:  gozfree <gozfree@163.com>
 * created: 2016-04-24 16:22:04
 * updated: 2016-04-24 16:22:04
 *****************************************************************************/
#ifndef LIBATOMIC_H
#define LIBATOMIC_H

#ifdef __cplusplus
extern "C" {
#endif

#define HAVE_ATOMICS_NATIVE 1
#define HAVE_ATOMICS_GCC    1

#if HAVE_ATOMICS_NATIVE

#if HAVE_ATOMICS_GCC
#include "libatomic_gcc.h"
#elif HAVE_ATOMICS_WIN32
#include "libatomic_win32.h"
#elif HAVE_ATOMICS_SUNCC
#include "libatomic_suncc.h"
#endif

#else

/**
 * Load the current value stored in an atomic integer.
 *
 * @param ptr atomic integer
 * @return the current value of the atomic integer
 * @note This acts as a memory barrier.
 */
int atomic_int_get(volatile int *ptr);

/**
 * Store a new value in an atomic integer.
 *
 * @param ptr atomic integer
 * @param val the value to store in the atomic integer
 * @note This acts as a memory barrier.
 */
void atomic_int_set(volatile int *ptr, int val);

/**
 * Add a value to an atomic integer.
 *
 * @param ptr atomic integer
 * @param inc the value to add to the atomic integer (may be negative)
 * @return the new value of the atomic integer.
 * @note This does NOT act as a memory barrier. This is primarily
 *       intended for reference counting.
 */
int atomic_int_add_and_fetch(volatile int *ptr, int inc);
int atomic_int_sub_and_fetch(volatile int *ptr, int inc);

int atomic_int_inc(volatile int *ptr);

int atomic_int_dec(volatile int *ptr);

/**
 * Atomic pointer compare and swap.
 *
 * @param ptr pointer to the pointer to operate on
 * @param oldval do the swap if the current value of *ptr equals to oldval
 * @param newval value to replace *ptr with
 * @return the value of *ptr before comparison
 */
void *atomic_ptr_cas(void * volatile *ptr, void *oldval, void *newval);

#endif /* HAVE_ATOMICS_NATIVE */


#ifdef __cplusplus
}
#endif
#endif
