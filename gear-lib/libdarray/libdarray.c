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
#include "libdarray.h"
#include <stdio.h>
#include <stdlib.h>

#define DARRAY_INVALID ((size_t)-1)

void darray_init(struct darray *dst)
{
    dst->array = NULL;
    dst->num = 0;
    dst->capacity = 0;
}

void darray_free(struct darray *dst)
{
    free(dst->array);
    dst->array = NULL;
    dst->num = 0;
    dst->capacity = 0;
}

size_t darray_alloc_size(const size_t element_size,
                const struct darray *da)
{
    return element_size * da->num;
}

static void *darray_item(const size_t element_size,
                const struct darray *da, size_t idx)
{
    return (void *)(((uint8_t *)da->array) + element_size * idx);
}

void *darray_end(const size_t element_size,
                const struct darray *da)
{
    if (!da->num)
            return NULL;

    return darray_item(element_size, da, da->num - 1);
}

void darray_reserve(const size_t element_size, struct darray *dst,
                const size_t capacity)
{
    void *ptr;
    if (capacity == 0 || capacity <= dst->num)
            return;

    ptr = malloc(element_size * capacity);
    if (dst->num)
            memcpy(ptr, dst->array, element_size * dst->num);
    if (dst->array)
            free(dst->array);
    dst->array = ptr;
    dst->capacity = capacity;
}

static void darray_ensure_capacity(const size_t element_size,
                struct darray *dst,
                const size_t new_size)
{
    size_t new_cap;
    void *ptr;
    if (new_size <= dst->capacity)
            return;

    new_cap = (!dst->capacity) ? new_size : dst->capacity * 2;
    if (new_size > new_cap)
            new_cap = new_size;
    ptr = malloc(element_size * new_cap);
    if (dst->capacity)
            memcpy(ptr, dst->array, element_size * dst->capacity);
    if (dst->array)
            free(dst->array);
    dst->array = ptr;
    dst->capacity = new_cap;
}

static void darray_resize(const size_t element_size, struct darray *dst,
                const size_t size)
{
    int b_clear;
    size_t old_num;

    if (size == dst->num) {
            return;
    } else if (size == 0) {
            dst->num = 0;
            return;
    }

    b_clear = size > dst->num;
    old_num = dst->num;

    darray_ensure_capacity(element_size, dst, size);
    dst->num = size;

    if (b_clear)
            memset(darray_item(element_size, dst, old_num), 0,
                            element_size * (dst->num - old_num));
}

static void darray_copy(const size_t element_size, struct darray *dst,
                const struct darray *da)
{
    if (da->num == 0) {
            darray_free(dst);
    } else {
            darray_resize(element_size, dst, da->num);
            memcpy(dst->array, da->array, element_size * da->num);
    }
}

static void darray_copy_array(const size_t element_size,
                struct darray *dst, const void *array,
                const size_t num)
{
    darray_resize(element_size, dst, num);
    memcpy(dst->array, array, element_size * dst->num);
}

void darray_move(struct darray *dst, struct darray *src)
{
    darray_free(dst);
    memcpy(dst, src, sizeof(struct darray));
    src->array = NULL;
    src->capacity = 0;
    src->num = 0;
}

size_t darray_find(const size_t element_size,
                const struct darray *da, const void *item,
                const size_t idx)
{
    size_t i;

    if (idx > da->num) {
            return DARRAY_INVALID;
    }

    for (i = idx; i < da->num; i++) {
            void *compare = darray_item(element_size, da, i);
            if (memcmp(compare, item, element_size) == 0)
                    return i;
    }

    return DARRAY_INVALID;
}

size_t darray_push_back(const size_t element_size,
                struct darray *dst, const void *item)
{
    darray_ensure_capacity(element_size, dst, ++dst->num);
    memcpy(darray_end(element_size, dst), item, element_size);

    return dst->num - 1;
}

static void *darray_push_back_new(const size_t element_size,
                struct darray *dst)
{
    void *last;

    darray_ensure_capacity(element_size, dst, ++dst->num);

    last = darray_end(element_size, dst);
    memset(last, 0, element_size);
    return last;
}

size_t darray_push_back_array(const size_t element_size,
                struct darray *dst,
                const void *array, const size_t num)
{
    size_t old_num;
    if (!dst)
            return 0;
    if (!array || !num)
            return dst->num;

    old_num = dst->num;
    darray_resize(element_size, dst, dst->num + num);
    memcpy(darray_item(element_size, dst, old_num), array,
                    element_size * num);

    return old_num;
}

static size_t darray_push_back_darray(const size_t element_size,
                struct darray *dst,
                const struct darray *da)
{
    return darray_push_back_array(element_size, dst, da->array, da->num);
}

void darray_insert(const size_t element_size, struct darray *dst,
                const size_t idx, const void *item)
{
    void *new_item;
    size_t move_count;

    if (idx > dst->num) {
            return;
    }

    if (idx == dst->num) {
            darray_push_back(element_size, dst, item);
            return;
    }

    move_count = dst->num - idx;
    darray_ensure_capacity(element_size, dst, ++dst->num);

    new_item = darray_item(element_size, dst, idx);

    memmove(darray_item(element_size, dst, idx + 1), new_item,
                    move_count * element_size);
    memcpy(new_item, item, element_size);
}

void *darray_insert_new(const size_t element_size,
                struct darray *dst, const size_t idx)
{
    void *item;
    size_t move_count;

    if (idx > dst->num) {
            return NULL;
    }
    if (idx == dst->num)
            return darray_push_back_new(element_size, dst);

    item = darray_item(element_size, dst, idx);

    move_count = dst->num - idx;
    darray_ensure_capacity(element_size, dst, ++dst->num);
    memmove(darray_item(element_size, dst, idx + 1), item,
                    move_count * element_size);

    memset(item, 0, element_size);
    return item;
}

static void darray_insert_array(const size_t element_size,
                struct darray *dst, const size_t idx,
                const void *array, const size_t num)
{
    size_t old_num;

    if (array == NULL)
            return;
    if (num == 0)
            return;
    if (idx >= dst->num)
            return;

    old_num = dst->num;
    darray_resize(element_size, dst, dst->num + num);

    memmove(darray_item(element_size, dst, idx + num),
                    darray_item(element_size, dst, idx),
                    element_size * (old_num - idx));
    memcpy(darray_item(element_size, dst, idx), array, element_size * num);
}

void darray_insert_darray(const size_t element_size,
                struct darray *dst, const size_t idx,
                const struct darray *da)
{
    darray_insert_array(element_size, dst, idx, da->array, da->num);
}

void darray_erase(const size_t element_size, struct darray *dst,
                const size_t idx)
{
    if (idx >= dst->num)
            return;

    if (idx >= dst->num || !--dst->num)
            return;

    memmove(darray_item(element_size, dst, idx),
                    darray_item(element_size, dst, idx + 1),
                    element_size * (dst->num - idx));
}

void darray_erase_item(const size_t element_size,
                struct darray *dst, const void *item)
{
    size_t idx = darray_find(element_size, dst, item, 0);
    if (idx != DARRAY_INVALID)
            darray_erase(element_size, dst, idx);
}

void darray_erase_range(const size_t element_size,
                struct darray *dst, const size_t start,
                const size_t end)
{
    size_t count, move_count;

    if (start > dst->num)
            return;
    if (end > dst->num)
            return;
    if (end <= start)
            return;

    count = end - start;
    if (count == 1) {
            darray_erase(element_size, dst, start);
            return;
    } else if (count == dst->num) {
            dst->num = 0;
            return;
    }

    move_count = dst->num - end;
    if (move_count)
            memmove(darray_item(element_size, dst, start),
                            darray_item(element_size, dst, end),
                            move_count * element_size);

    dst->num -= count;
}

void darray_pop_back(const size_t element_size,
                struct darray *dst)
{
    if (dst->num == 0)
            return;

    if (dst->num)
            darray_erase(element_size, dst, dst->num - 1);
}

void darray_join(const size_t element_size, struct darray *dst,
                struct darray *da)
{
    darray_push_back_darray(element_size, dst, da);
    darray_free(da);
}

void darray_split(const size_t element_size, struct darray *dst1,
                struct darray *dst2, const struct darray *da,
                const size_t idx)
{
    struct darray temp;

    if (da->num < idx)
            return;
    if (dst1 == dst2)
            return;

    darray_init(&temp);
    darray_copy(element_size, &temp, da);

    darray_free(dst1);
    darray_free(dst2);

    if (da->num) {
            if (idx)
                    darray_copy_array(element_size, dst1, temp.array,
                                    temp.num);
            if (idx < temp.num - 1)
                    darray_copy_array(element_size, dst2,
                                    darray_item(element_size, &temp, idx),
                                    temp.num - idx);
    }

    darray_free(&temp);
}

void darray_move_item(const size_t element_size,
                struct darray *dst, const size_t from,
                const size_t to)
{
    void *temp, *p_from, *p_to;

    if (from == to)
            return;

    temp = malloc(element_size);
    p_from = darray_item(element_size, dst, from);
    p_to = darray_item(element_size, dst, to);

    memcpy(temp, p_from, element_size);

    if (to < from)
            memmove(darray_item(element_size, dst, to + 1), p_to,
                            element_size * (from - to));
    else
            memmove(p_from, darray_item(element_size, dst, from + 1),
                            element_size * (to - from));

    memcpy(p_to, temp, element_size);
    free(temp);
}

void darray_swap(const size_t element_size, struct darray *dst,
                const size_t a, const size_t b)
{
    void *temp, *a_ptr, *b_ptr;

    if (a >= dst->num)
            return;
    if (b >= dst->num)
            return;

    if (a == b)
            return;

    temp = malloc(element_size);
    a_ptr = darray_item(element_size, dst, a);
    b_ptr = darray_item(element_size, dst, b);

    memcpy(temp, a_ptr, element_size);
    memcpy(a_ptr, b_ptr, element_size);
    memcpy(b_ptr, temp, element_size);

    free(temp);
}
