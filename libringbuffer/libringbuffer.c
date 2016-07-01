/******************************************************************************
 * Copyright (C) 2014-2015
 * file:    libringbuffer.c
 * author:  gozfree <gozfree@163.com>
 * created: 2016-06-29 11:42:50
 * updated: 2016-06-29 11:42:50
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libringbuffer.h"

#define MIN(a, b)           ((a) > (b) ? (b) : (a))
#define MAX(a, b)           ((a) > (b) ? (a) : (b))
#define CALLOC(size, type)  (type *)calloc(size, sizeof(type))

static int rb_get_space_free(struct ringbuffer *rb)
{
    if (!rb) {
        return -1;
    }
    return rb->length - rb->end -1;
}

static size_t rb_get_space_stored(struct ringbuffer *rb)
{
    if (!rb) {
        return -1;
    }
    return rb->end % rb->length - rb->start;
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
    int left = rb_get_space_free(rb);
    if ((int)len > left) {
        printf("Not enough space: %zu request, %d available\n", len, left);
        return -1;
    }
    memcpy(rb_end_ptr(rb), buf, len);
    rb->end = (rb->end + len) % rb->length;
    return len;
}

ssize_t rb_read(struct ringbuffer *rb, void *buf, size_t len)
{
    if (!rb) {
        return -1;
    }
    size_t rlen = MIN(len, rb_get_space_stored(rb));
    memcpy(buf, rb_start_ptr(rb), rlen);
    rb->start = (rb->start + rlen) % rb->length;
    if ((rb->start == rb->end) || (rb_get_space_stored(rb) == 0)) {
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
    size_t len = rb_get_space_stored(rb);
    if (len > 0) {
        buf = calloc(1, len);
        if (!buf) {
            printf("malloc %zu failed!\n", len);
            return NULL;
        }
    }
    *blen = len;
    memcpy(buf, rb_start_ptr(rb), len);
    return buf;
}

void rb_cleanup(struct ringbuffer *rb)
{
    if (!rb) {
        return;
    }
    rb->start = rb->end = 0;
}
