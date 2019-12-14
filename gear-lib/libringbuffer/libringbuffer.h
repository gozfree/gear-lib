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
#ifndef LIBRINGBUFFER_H
#define LIBRINGBUFFER_H

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ringbuffer {
    void *buffer;
    int length;
    size_t start;
    size_t end;
} ringbuffer;

struct ringbuffer *rb_create(int len);
void rb_destroy(struct ringbuffer *rb);
ssize_t rb_write(struct ringbuffer *rb, const void *buf, size_t len);
ssize_t rb_read(struct ringbuffer *rb, void *buf, size_t len);
void *rb_dump(struct ringbuffer *rb, size_t *len);
void rb_cleanup(struct ringbuffer *rb);
size_t rb_get_space_free(struct ringbuffer *rb);
size_t rb_get_space_used(struct ringbuffer *rb);

#ifdef __cplusplus
}
#endif
#endif
