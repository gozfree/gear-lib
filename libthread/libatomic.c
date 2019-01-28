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
#include "libatomic.h"

#define GCC_VERSION (__GNUC__*100 + __GNUC_MINOR__*10 + __GNUC_PATCHLEVEL__)

#if GCC_VERSION > 463
#define HAVE_ATOMIC_COMPARE_EXCHANGE 1
#else
#define HAVE_ATOMIC_COMPARE_EXCHANGE 0
#endif

#define HAVE_ATOMICS_NATIVE 1
#define HAVE_SYNC_VAL_COMPARE_AND_SWAP 1

#if HAVE_THREADS
#include <pthread.h>
static pthread_mutex_t atomic_lock = PTHREAD_MUTEX_INITIALIZER;
#endif

inline int atomic_int_get(volatile int *ptr)
{
#if HAVE_ATOMICS_NATIVE
  #if HAVE_ATOMIC_COMPARE_EXCHANGE
    return __atomic_load_n(ptr, __ATOMIC_SEQ_CST);
  #else
    __sync_synchronize();
    return *ptr;
  #endif
#elif HAVE_THREADS
    int res;
    pthread_mutex_lock(&atomic_lock);
    res = *ptr;
    pthread_mutex_unlock(&atomic_lock);
    return res;
#else
    return *ptr;
#endif

}

inline void atomic_int_set(volatile int *ptr, int val)
{
#if HAVE_ATOMICS_NATIVE
  #if HAVE_ATOMIC_COMPARE_EXCHANGE
    __atomic_store_n(ptr, val, __ATOMIC_SEQ_CST);
  #else
    *ptr = val;
    __sync_synchronize();
  #endif
#elif HAVE_THREADS
    pthread_mutex_lock(&atomic_lock);
    *ptr = val;
    pthread_mutex_unlock(&atomic_lock);
#else
    *ptr = val;
#endif
}

inline int atomic_int_add_and_fetch(volatile int *ptr, int inc)
{
#if HAVE_ATOMICS_NATIVE
  #if HAVE_ATOMIC_COMPARE_EXCHANGE
    return __atomic_add_fetch(ptr, inc, __ATOMIC_SEQ_CST);
  #else
    return __sync_add_and_fetch(ptr, inc);
  #endif
#elif HAVE_THREADS
    int res;
    pthread_mutex_lock(&atomic_lock);
    *ptr += inc;
    res = *ptr;
    pthread_mutex_unlock(&atomic_lock);
    return res;
#else
    *ptr += inc;
    return *ptr;
#endif
}

inline int atomic_int_sub_and_fetch(volatile int *ptr, int inc)
{
#if HAVE_ATOMICS_NATIVE
  #if HAVE_ATOMIC_COMPARE_EXCHANGE
    return __atomic_sub_fetch(ptr, inc, __ATOMIC_SEQ_CST);
  #else
    return __sync_sub_and_fetch(ptr, inc);
  #endif
#elif HAVE_THREADS
    int res;
    pthread_mutex_lock(&atomic_lock);
    *ptr -= inc;
    res = *ptr;
    pthread_mutex_unlock(&atomic_lock);
    return res;
#else
    *ptr -= inc;
    return *ptr;
#endif
}

inline void *atomic_ptr_cas(void * volatile *ptr,
                            void *oldval, void *newval)
{
#if HAVE_ATOMICS_NATIVE
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
#elif HAVE_THREADS
    void *ret;
    pthread_mutex_lock(&atomic_lock);
    ret = *ptr;
    if (ret == oldval)
        *ptr = newval;
    pthread_mutex_unlock(&atomic_lock);
    return ret;
#else
    if (*ptr == oldval) {
        *ptr = newval;
        return oldval;
    }
    return *ptr;
#endif
}

inline int atomic_int_inc(volatile int *ptr)
{
    return atomic_int_add_and_fetch(ptr, 1);
}

inline int atomic_int_dec(volatile int *ptr)
{
    return atomic_int_add_and_fetch(ptr, -1);
}
