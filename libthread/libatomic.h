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
#ifndef LIBATOMIC_H
#define LIBATOMIC_H

#ifdef __cplusplus
extern "C" {
#endif

#define HAVE_ATOMICS_NATIVE 1
#define HAVE_ATOMICS_GCC    1

#if HAVE_ATOMICS_NATIVE

#if HAVE_ATOMICS_GCC
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


#elif HAVE_ATOMICS_WIN32
//#include "libatomic_win32.h"
#elif HAVE_ATOMICS_SUNCC
//#include "libatomic_suncc.h"
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
