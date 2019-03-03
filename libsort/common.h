/*****************************************************************************
 * file:    common.h
 * author:  xw_y_am <xw_y_am@163.com>
 * created: 2019-03-03
 * description: libsort inner common functions
 *****************************************************************************/

#ifndef COMMON_H
#define COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>

typedef unsigned char byte;
typedef int (*fp_cmp)(const byte *, const byte *, size_t);
typedef void (*fp_swap)(const byte *, const byte *, size_t);

static int default_cmp(const byte *p, const byte *q, size_t size)
{
    for (; size--; p++, q++) {
        if (*p != *q) return *p - *q;
    }
    return 0;
}

static inline void default_swap(byte *p, byte *q, size_t size)
{
    while (size--) {
        byte t = *p;
        *p++ = *q;
        *q++ = t;
    }
}

#ifdef __cplusplus
}
#endif
#endif 
