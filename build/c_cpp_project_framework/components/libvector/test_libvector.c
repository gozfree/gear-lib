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
#include "libvector.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <unistd.h>

struct tmp_box {
    char c;
    int i;
    float f;
};

void mix_struct()
{
    struct tmp_box tb;
    vector_iter iter;
    vector_t *c;
    tb.c = 'a';
    tb.i = 1;
    tb.f = 1.23;
#if defined (__linux__) || defined (__CYGWIN__)
    c = vector_create(struct tmp_box);
#else
    c = _vector_create(sizeof(struct tmp_box));
#endif
    vector_push_back(c, tb);
    for (iter = vector_begin(c); iter != vector_end(c); iter = vector_next(c)) {
        struct tmp_box *tt = vector_iter_valuep(c, iter, struct tmp_box);
        printf("vector member.c: %c\n", tt->c);
        printf("vector member.i: %d\n", tt->i);
        printf("vector member.f: %f\n", tt->f);
    }
    printf("size = %zu\n", c->size);
    printf("type_size = %zu\n", c->type_size);
    printf("max_size = %zu\n", c->max_size);
    printf("capacity = %zu\n", c->capacity);
    vector_destroy(c);

}

void default_struct()
{
    int i;
    int sum = 0;
    int *tmp = (int *)calloc(1, sizeof(int));
#if defined (__linux__) || defined (__CYGWIN__)
#else
    int itmp = 0;
#endif
    int t1 = 100, t2 = 200, t3 = 300;
    vector_t *a;
    vector_iter iter;
#if defined (__linux__) || defined (__CYGWIN__)
    a = vector_create(int);
#else
    a = _vector_create(sizeof(int));
#endif
    vector_push_back(a, t1);
    vector_push_back(a, t2);
    vector_push_back(a, t3);
    for (iter = vector_begin(a); iter != vector_end(a); iter = vector_next(a)) {
        printf("vector member: %d\n", *vector_iter_valuep(a, iter, int));
    }
    for (i = 0; i < a->size; i++) {
        printf("vector member: %d\n", *vector_at(a, i, int));
    }

    while (!vector_empty(a)) {
#if defined (__linux__) || defined (__CYGWIN__)
        memcpy(tmp, vector_back(a, int), sizeof(int));
#else
        memcpy(itmp, vector_last(a), a->type_size);
        tmp = &itmp;
#endif
        sum += *tmp;
        vector_pop_back(a);
    }
    free(tmp);
    printf("sum is %d\n", sum);
    printf("size = %zu\n", a->size);
    printf("type_size = %zu\n", a->type_size);
    printf("max_size = %zu\n", a->max_size);
    printf("capacity = %zu\n", a->capacity);
    vector_destroy(a);
}

int main(int argc, char **argv)
{
    mix_struct();
    default_struct();
    return 0;
}
