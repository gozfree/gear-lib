/*****************************************************************************
 * Copyright (C) 2014-2015
 * file:    libvector.h
 * author:  gozfree <gozfree@163.com>
 * created: 2015-09-16 00:48
 * updated: 2015-09-16 00:48
 *****************************************************************************/
#ifndef _LIBVECTOR_H_
#define _LIBVECTOR_H_

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

typedef struct vector {
    size_t type_size;
    size_t (*size)();
    size_t (*max_size)();
    size_t (*capacity)();
    bool (*empty)();
    void (*push_back)(void *e);
    void *(*begin)();
    void *(*end)();

} vector_t;

struct vector *vector_init();

#define VECTOR(type, name) \
        struct vector *name = vector_init(sizeof(type))



#ifdef __cplusplus
}
#endif
#endif
