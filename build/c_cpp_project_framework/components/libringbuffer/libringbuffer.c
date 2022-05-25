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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libringbuffer.h"

#define MIN(a, b)           ((a) > (b) ? (b) : (a))
#define MAX(a, b)           ((a) > (b) ? (a) : (b))
#define CALLOC(size, type)  (type *)calloc(size, sizeof(type))

size_t rb_get_space_free(struct ringbuffer *rb)
{
    if (!rb) {
        return -1;
    }
    if (rb->end >= rb->start) {
        return rb->length - (rb->end - rb->start)-1;
    } else {
        return rb->start - rb->end-1;
    }
}

size_t rb_get_space_used(struct ringbuffer *rb)
{
    if (!rb) {
        return -1;
    }
    if (rb->end >= rb->start) {
        return rb->end - rb->start;
    } else {
        return rb->length - (rb->start - rb->end);
    }
}

struct ringbuffer *rb_create(int len)
{
    struct ringbuffer *rb = CALLOC(1, struct ringbuffer);
    if (!rb) {
        printf("malloc ringbuffer failed!\n");
        return NULL;
    }
    rb->length = len + 1;
    rb->start = 0;
    rb->end = 0;
    rb->buffer = calloc(1, rb->length);
    if (!rb->buffer) {
        printf("malloc rb->buffer failed!\n");
        free(rb);
        return NULL;
    }
    return rb;
}

void rb_destroy(struct ringbuffer *rb)
{
    if (!rb) {
        return;
    }
    free(rb->buffer);
    free(rb);
}

void *rb_end_ptr(struct ringbuffer *rb)
{
    return (void *)((char *)rb->buffer + rb->end);
}

void *rb_start_ptr(struct ringbuffer *rb)
{
    return (void *)((char *)rb->buffer + rb->start);
}

ssize_t rb_write(struct ringbuffer *rb, const void *buf, size_t len)
{
    if (!rb) {
        return -1;
    }
    size_t left = rb_get_space_free(rb);
    if (len > left) {
        printf("Not enough space: %zu request, %zu available\n", len, left);
        return -1;
    }

    if ((rb->length - rb->end) < len) {
        int half_tail = rb->length - rb->end;
        memcpy(rb_end_ptr(rb), buf, half_tail);
        rb->end = (rb->end + half_tail) % rb->length;

        int half_head = len - half_tail;
        memcpy(rb_end_ptr(rb), buf+half_tail, half_head);
        rb->end = (rb->end + half_head) % rb->length;
    } else {
        memcpy(rb_end_ptr(rb), buf, len);
        rb->end = (rb->end + len) % rb->length;
    }
    return len;
}

ssize_t rb_read(struct ringbuffer *rb, void *buf, size_t len)
{
    if (!rb) {
        return -1;
    }
    size_t rlen = MIN(len, rb_get_space_used(rb));

    if ((rb->length - rb->start) < rlen) {
        int half_tail = rb->length - rb->start;
        memcpy(buf, rb_start_ptr(rb), half_tail);
        rb->start = (rb->start + half_tail) % rb->length;

        int half_head = rlen - half_tail;
        memcpy(buf+half_tail, rb_start_ptr(rb), half_head);
        rb->start = (rb->start + half_head) % rb->length;
    } else {
        memcpy(buf, rb_start_ptr(rb), rlen);
        rb->start = (rb->start + rlen) % rb->length;
    }

    if ((rb->start == rb->end) || (rb_get_space_used(rb) == 0)) {
        rb->start = rb->end = 0;
    }
    return rlen;
}

void *rb_dump(struct ringbuffer *rb, size_t *blen)
{
    if (!rb) {
        return NULL;
    }
    void *buf = NULL;
    size_t len = rb_get_space_used(rb);
    if (len <= 0) {
        return NULL;
    }
    buf = calloc(1, len);
    if (!buf) {
        printf("malloc %zu failed!\n", len);
        return NULL;
    }
    *blen = len;

    if ((rb->length - rb->start) < len) {
        int half_tail = rb->length - rb->start;
        memcpy(buf, rb_start_ptr(rb), half_tail);

        int half_head = len - half_tail;
        memcpy(buf+half_tail, rb->buffer, half_head);
    } else {
        memcpy(buf, rb_start_ptr(rb), len);
    }
    return buf;
}

void rb_cleanup(struct ringbuffer *rb)
{
    if (!rb) {
        return;
    }
    rb->start = rb->end = 0;
}
