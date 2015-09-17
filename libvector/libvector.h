/*****************************************************************************
 * Copyright (C) 2014-2015
 * file:    libvector.h
 * author:  gozfree <gozfree@163.com>
 * created: 2015-09-16 00:48
 * updated: 2015-09-16 00:48
 *****************************************************************************/
#ifndef _LIBVECTOR_H_
#define _LIBVECTOR_H_

#include <sys/uio.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef bool
#define bool int
#endif

#ifndef true
#define true (1==1)
#endif

#ifndef false
#define false (0==1)
#endif

struct vector;

struct vector *vector_init();
void push_back(struct vector *v, void *e);

#define vector_push_back(v, e) push_back(v, (void *)&e)
#define vector_pop_back(v)  pop_back(v)
#define vector_back(v) \
    (__builtin_types_compatible_p(typeof(e), int) \
    ? back_int(v) \
    : __builtin_types_compatible_p(typeof(e), long) \
    ? push_back_long(v, e) \
    : __builtin_types_compatible_p(typeof(e), float) \
    ? push_back_float(v, e)	\
    : printf("not support type\n"))



typedef struct vector {
    int type;
    size_t type_size;
    struct iovec buf;

    size_t (*size)();
    size_t (*max_size)();
    size_t (*capacity)();
    bool (*_empty)();
    //void (*push_back)(struct vector *v, void *e);
    void *(*_begin)();
    void *(*_end)();

} vector_t;
enum {
    _voidp       = 0,
    _char        = 1,
    _charp       = 1,
    _uchar        = 1,
    _ucharp        = 1,
    _short       = 2,
    _shortp      = 2,
    _ushort       = 2,
    _ushortp       = 2,
    _int         = 3,
    _intp         = 3,
    _uint         = 3,
    _uintp         = 3,
    _long        = 4,
    _longp        = 4,
    _ulong        = 4,
    _ulongp        = 4,
    _longlong    = 5,
    _longlongp    = 5,
    _ulonglong    = 5,
    _ulonglongp    = 5,
    _int8_t      = 6,
    _int8_tp      = 6,
    _uint8_t     = 7,
    _uint8_tp     = 7,
    _int16_t     = 8,
    _int16_tp     = 8,
    _uint16_t    = 9,
    _uint16_tp    = 9,
    _int32_t     = 10,
    _int32_tp     = 10,
    _uint32_t    = 11,
    _uint32_tp    = 11,
    _int64_t     = 12,
    _int64_tp     = 12,
    _uint64_t    = 13,
    _uint64_tp    = 13,
    _float       = 14,
    _floatp       = 14,
    _double      = 15,
    _doublep      = 15,
};


#define VECTOR(T, V) \
    struct vector *V = vector_init(sizeof(T)); \
    __builtin_types_compatible_p(T, void *) ? V->type   = _voidp \
    : __builtin_types_compatible_p(T, char) ? V->type   = _char \
    : __builtin_types_compatible_p(T, int) ? V->type    = _int \
    : __builtin_types_compatible_p(T, long) ? V->type   = _long \
    : printf("not support type\n");

#ifdef __cplusplus
}
#endif
#endif
