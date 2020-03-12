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
#ifndef LIBVECTOR_H
#define LIBVECTOR_H

#include <stdint.h>
#if defined (__linux__) || defined (__CYGWIN__)
#include <unistd.h>
#include <sys/uio.h>
#elif defined (__WIN32__) || defined (WIN32) || defined (_MSC_VER)
#include "libposix4win.h"
#endif

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
vector_iter vector_last(struct vector *v);//last=end-1
vector_iter vector_next(struct vector *v);
vector_iter vector_prev(struct vector *v);
void *_vector_iter_value(struct vector *v, vector_iter iter);
void *_vector_at(struct vector *v, int pos);


#if defined (__linux__) || defined (__CYGWIN__)
#define vector_create(type_t) \
    ({type_t t;struct vector *v = _vector_create(sizeof(t)); v;})
#endif
void vector_destroy(struct vector *v);
int vector_empty(struct vector *v);
#define vector_push_back(v, e) _vector_push_back(v, (void *)&e, sizeof(e))
void vector_pop_back(struct vector *v);
#if defined (__linux__) || defined (__CYGWIN__)
#define vector_back(v, type_t) \
    ({ \
        type_t __tmp; \
        memcpy(&__tmp, vector_last(v), v->type_size); \
        &__tmp; \
    })
#endif

#define vector_iter_valuep(vector, iter, type_t) \
    (type_t *)_vector_iter_value(vector, iter)

#define vector_at(v, pos, type_t) \
    (type_t *)_vector_at(v, pos)


#ifdef __cplusplus
}
#endif
#endif
