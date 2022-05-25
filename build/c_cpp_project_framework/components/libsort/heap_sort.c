/*****************************************************************************
 * Copyright (C) 2014-2015
 * file:    heap_sort.c
 * author:  gozfree <gozfree@163.com>
 * created: 2015-08-09 18:02
 * updated: 2015-08-09 18:02
 *****************************************************************************/
#include "libsort.h"

static void u32_swap(void *a, void *b, size_t size)
{
    uint32_t t = *(uint32_t *)a;
    *(uint32_t *)a = *(uint32_t *)b;
    *(uint32_t *)b = t;
}

static void generic_swap(void *a, void *b, size_t size)
{
    char t;
    do {
        t = *(char *)a;
        *(char *)a++ = *(char *)b;
        *(char *)b++ = t;
    } while (--size > 0);
}

static int generic_cmp(const void *a, const void *b, size_t size)
{
    const unsigned char *p = (const unsigned char *)a;
    const unsigned char *q = (const unsigned char *)b;
    for (; size--; p++, q++) {
        if (*p != *q) return *p - *q;
    }
    return 0;
}


/**
 * sort - sort an array of elements
 * @base: pointer to data to sort
 * @num: number of elements
 * @size: size of each element
 * @cmp_func: pointer to comparison function
 * @swap_func: pointer to swap function or NULL
 *
 * This function does a heapsort on the given array. You may provide a
 * swap_func function optimized to your element type.
 *
 * Sorting time is O(n log n) both on average and worst-case. While
 * qsort is about 20% faster on average, it suffers from exploitable
 * O(n*n) worst-case behavior and extra memory requirements that make
 * it less suitable for kernel use.
 */

static void sort(void *base, size_t num, size_t size,
          int (*cmp_func)(const void *, const void *, size_t),
          void (*swap_func)(void *, void *, size_t))
{
    /* pre-scale counters for performance */
    int i = (num/2 - 1) * size, n = num * size, c, r;

    if (!swap_func)
        swap_func = (size == 4 ? u32_swap : generic_swap);
    if (!cmp_func)
        cmp_func = generic_cmp;

    /* heapify */
    for ( ; i >= 0; i -= size) {
        for (r = i; r * 2 + size < n; r  = c) {
            c = r * 2 + size;
            if (c < n - size &&
                cmp_func(base + c, base + c + size, size) < 0)
                c += size;
            if (cmp_func(base + r, base + c, size) >= 0)
                break;
            swap_func(base + r, base + c, size);
        }
    }

    /* sort */
    for (i = n - size; i > 0; i -= size) {
        swap_func(base, base + i, size);
        for (r = 0; r * 2 + size < i; r = c) {
            c = r * 2 + size;
            if (c < i - size &&
                cmp_func(base + c, base + c + size, size) < 0)
                c += size;
            if (cmp_func(base + r, base + c, size) >= 0)
                break;
            swap_func(base + r, base + c, size);
        }
    }
}

void heap_sort(void *base, size_t num, size_t size, fp_cmp cmp)
{
    sort(base, num, size, cmp, NULL);
}
