/*****************************************************************************
 * Copyright (C) 2014-2015
 * file:    libvector.h
 * author:  gozfree <gozfree@163.com>
 * created: 2015-09-16 00:48
 * updated: 2015-09-16 00:48
 *****************************************************************************/
#ifndef LIBVECTOR_H
#define LIBVECTOR_H

#include <stdint.h>
#include <sys/uio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void *vector_iter;

typedef struct vector {
    size_t size;     //number of element
    size_t max_size; //max number of element
    size_t capacity; //size of allocated storage capacity
    size_t type_size;
    size_t tmp_cursor;
    struct iovec buf;
    vector_iter iterator;
} vector_t;

/*
 * vector_assign
 * vector_create
 * vector_destroy
 * vector_empty
 * vector_push_back
 * vector_pop_back
 * vector_back
 * vector_begin
 * vector_end
 * vector_size
 * vector_iter_valuep
 * vector_at
 * vector_next
 * vector_prev
*/

/*
 * inner apis
 */
struct vector *_vector_create(size_t size);
void _vector_push_back(struct vector *v, void *e, size_t type_size);
vector_iter vector_begin(struct vector *v);
vector_iter vector_end(struct vector *v);
vector_iter vector_next(struct vector *v);
vector_iter vector_prev(struct vector *v);
void *_vector_iter_value(struct vector *v, vector_iter iter);
void *_vector_at(struct vector *v, int pos);


#define vector_create(type_t) \
    ({type_t t;struct vector *v = _vector_create(sizeof(t)); v;})
void vector_destroy(struct vector *v);
int vector_empty(struct vector *v);
#define vector_push_back(v, e) _vector_push_back(v, (void *)&e, sizeof(e))
void vector_pop_back(struct vector *v);
#define vector_back(v, type_t) \
    ({ \
        type_t tmp; \
        memcpy(&tmp, vector_end(v), v->type_size); \
        &tmp; \
    })

#define vector_iter_valuep(vector, iter, type_t) \
    (type_t *)_vector_iter_value(vector, iter)

#define vector_at(v, pos, type_t) \
    (type_t *)_vector_at(v, pos)


#ifdef __cplusplus
}
#endif
#endif
