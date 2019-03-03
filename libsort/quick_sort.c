/*****************************************************************************
 * file:    quick_sort.c
 * author:  xw_y_am <xw_y_am@163.com>
 * created: 2019-03-03
 *****************************************************************************/
#include "common.h"

static void qsort_recu(byte *l, byte *r, size_t size)
{
    if (l < r) {
        byte *p = l;
        byte *q = r;
        
        do {
            while ((p < q) && (default_cmp(p, q, size) <= 0)) 
                q -= size;

            if (p != q) {
                default_swap(p, q, size);
                p += size;
            }

            while ((p < q) && (default_cmp(p, q, size) <= 0))
                p += size;

            if (p != q) {
                default_swap(q, p, size);
                q -= size;
            }
        } while (p < q);

        qsort_recu(l, p - size, size);
        qsort_recu(p + size, r, size);
    }
}

int quick_sort(void *array, size_t num, size_t size)
{
    if (!array || !num || !size) {
        return -1;
    }
    qsort_recu((byte *)array, (byte *)array + size * (num - 1), size);
    return 0;
}
