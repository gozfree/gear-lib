/*****************************************************************************
 * file:    quick_sort.c
 * author:  xw_y_am <xw_y_am@163.com>
 * created: 2019-03-03
 *****************************************************************************/
#include "common.h"

static void qsort_recu(byte *l, byte *r, size_t size, fp_cmp cmp)
{
    if (l < r) {
        byte *p = l;
        byte *q = r;

        do {
            while ((p < q) && (cmp(p, q, size) <= 0))
                q -= size;

            if (p != q) {
                byte_swap(p, q, size);
                p += size;
            }

            while ((p < q) && (cmp(p, q, size) <= 0))
                p += size;

            if (p != q) {
                byte_swap(q, p, size);
                q -= size;
            }
        } while (p < q);

        qsort_recu(l, p - size, size, cmp);
        qsort_recu(p + size, r, size, cmp);
    }
}

int quick_sort(void *array, size_t num, size_t size, fp_cmp cmp)
{
    CHK_PARAMETERS(array, num, size, cmp);
    qsort_recu((byte *)array, (byte *)array + size * (num - 1), size, cmp);
    return 0;
}
