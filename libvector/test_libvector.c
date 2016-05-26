/*****************************************************************************
 * Copyright (C) 2014-2015
 * file:    test_libvector.c
 * author:  gozfree <gozfree@163.com>
 * created: 2015-05-31 19:42
 * updated: 2015-05-31 19:42
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "libvector.h"

struct tmp_box {
    char c;
    int i;
    float f;
};

void mix_struct()
{
    struct tmp_box tb;
    vector_iter iter;
    tb.c = 'a';
    tb.i = 1;
    tb.f = 1.23;
    vector_t *c = vector_create(struct tmp_box);
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
    int sum = 0;
    int *tmp;
    int t1 = 100, t2 = 200, t3 = 300;
    vector_iter iter;
    vector_t *a = vector_create(int);
    vector_push_back(a, t1);
    vector_push_back(a, t2);
    vector_push_back(a, t3);
    for (iter = vector_begin(a); iter != vector_end(a); iter = vector_next(a)) {
        printf("vector member: %d\n", *vector_iter_valuep(a, iter, int));
    }

    while (!vector_empty(a)) {
        tmp = vector_back(a, int);
        sum += *tmp;
        vector_pop_back(a);
    }
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
