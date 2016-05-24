/*****************************************************************************
 * Copyright (C) 2014-2015
 * file:    libvector.h
 * author:  gozfree <gozfree@163.com>
 * created: 2015-09-16 00:48
 * updated: 2015-09-16 00:48
 *****************************************************************************/
#ifndef _LIBVECTOR_H_
#define _LIBVECTOR_H_

#include <libgzf.h>
#include <sys/uio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum type_arg {
    _voidp    =  0,
    _char     =  1, _charp    , _uchar    , _ucharp    ,
    _short    =  5, _shortp   , _ushort   , _ushortp   ,
    _int      =  9, _intp     , _uint     , _uintp     ,
    _long     = 13, _longp    , _ulong    , _ulongp    ,
    _longlong = 17, _longlongp, _ulonglong, _ulonglongp,
    _int8_t   = 21, _int8_tp  , _uint8_t  , _uint8_tp  ,
    _int16_t  = 25, _int16_tp , _uint16_t , _uint16_tp ,
    _int32_t  = 29, _int32_tp , _uint32_t , _uint32_tp ,
    _int64_t  = 33, _int64_tp , _uint64_t , _uint64_tp ,
    _float    = 37, _floatp   , _double   , _doublep   ,

    _unknown
} type_arg_t;

#define TYPE_ARG(t) \
    (__builtin_types_compatible_p(t, void *) ? _voidp \
    :__builtin_types_compatible_p(t, char) ? _char \
    :__builtin_types_compatible_p(t, int) ? _int \
    :__builtin_types_compatible_p(t, long) ? _long \
    : _unknown)

typedef struct vector {
    type_arg_t type;
    size_t size;     //number of element 
    size_t max_size; //max number of element
    size_t capacity; //size of allocated storage capacity
    size_t type_size;
    size_t tmp_cursor;
    struct iovec buf;
} vector_t;

/*
 * inner apis
 */
struct vector *init();
void push_back(struct vector *v, void *e);
void *_vector_begin(struct vector *v);
void *_vector_end(struct vector *v);
void *_vector_plusplus(struct vector *v);

/*
 * vector_new
 * vector_empty
 * vector_push_back
 * vector_pop_back
 * vector_back
 * vector_begin
 * vector_end
 * vector_plusplus
*/
#define vector_new(t) \
    ({struct vector *v = init(TYPE_ARG(t), sizeof(t)); v;})
int vector_empty(struct vector *v);
#define vector_push_back(v, e) push_back(v, (void *)&e)
void vector_pop_back(struct vector *v);
#define vector_back(v, type_t) \
    ({ \
        type_t tmp; \
        memcpy(&tmp, _vector_end(v), v->type_size); \
        &tmp; \
    })
#define vector_begin(v, type_t) \
    *(type_t *)_vector_begin(v)
#define vector_end(v, type_t) \
    *(type_t *)_vector_end(v)
#define vector_plusplus(v, type_t) \
    *(type_t *)_vector_plusplus(v)

#if 0
#define vector_get_member(v, pos, type_t) \
    (type_t *)get_member(v, pos)
#else
//XXX: need debug
#define vector_get_member(v, pos, type_t) \
    (type_t *)(v->buf.iov_base + pos * v->type_size)
#endif


#ifdef __cplusplus
}
#endif
#endif
