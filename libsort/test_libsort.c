/*****************************************************************************
 * Copyright (C) 2014-2015
 * file:    test_libsort.c
 * author:  gozfree <gozfree@163.com>
 * created: 2015-08-09 01:20
 * updated: 2015-08-09 01:20
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "libsort.h"

#define print_array(type, format, array) \
    do {\
        size_t len = sizeof(array)/sizeof(array[0]);\
        int _i = 0;\
        for (_i = 0; _i < len; ++_i) {\
            printf(format, (type)(*((type *)array+_i)));\
        }\
        printf("\n");\
    } while (0)


void test_heapsort()
{
    int a[]={4,1,2,5,3, 2, 2, 2, 1};
    print_array(int, "%d\t", a);
    heap_sort(a, sizeof(a)/sizeof(a[0]), sizeof(int));
    print_array(int, "%d\t", a);

    int b[]={8,1,2,5,3};
    print_array(int, "%d\t", b);
    heap_sort(b, sizeof(b)/sizeof(b[0]), sizeof(int));
    print_array(int, "%d\t", b);

    float f[]={1.1,2.2,4.2,3.0};
    print_array(float, "%f\t", f);
    heap_sort(f, sizeof(f)/sizeof(f[0]), sizeof(float));
    print_array(float, "%f\t", f);
}

void test_bsort()
{
#if 1
    int a[]={4,1,2,5,3};
    print_array(int, "%d\t", a);
    //bsort(int, a);
    bubble_sort(a, sizeof(a)/sizeof(a[0]), sizeof(int));
    print_array(int, "%d\t", a);

    int b[]={8,1,2,5,3};
    print_array(int, "%d\t", b);
    bubble_sort(b, sizeof(b)/sizeof(b[0]), sizeof(int));
    print_array(int, "%d\t", b);
#endif

    float f[]={1.1,2.2,4.2,3.0};
    print_array(float, "%f\t", f);
    bubble_sort(f, sizeof(f)/sizeof(f[0]), sizeof(float));
    print_array(float, "%f\t", f);

}
int main(int argc, char **argv)
{
    test_bsort();
    test_heapsort();
    return 0;
}
