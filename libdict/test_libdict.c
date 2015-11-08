/*****************************************************************************
 * Copyright (C) 2014-2015
 * file:    test_libdict.c
 * author:  gozfree <gozfree@163.com>
 * created: 2015-04-29 00:45
 * updated: 2015-07-13 03:30
 *****************************************************************************/
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "libdict.h"

#define ALIGN   "%15s: %6.4f sec\n"
#define NKEYS   1024*1024
//#define NKEYS   10

double epoch_double()
{
    struct timeval t;
    gettimeofday(&t, NULL);
    return t.tv_sec + (t.tv_usec * 1.0) / 1000000.0;
}

int main(int argc, char * argv[])
{
    dict * d;
    double t1, t2;
    int i;
    int nkeys;
    char * buffer;
    char * val;

    nkeys = (argc>1) ? (int)atoi(argv[1]) : NKEYS;
    printf("%15s: %d\n", "values", nkeys);
    buffer = malloc(9 * nkeys);
    if (!buffer) {
        printf("malloc failed!\n");
        return -1;
    }

    d = dict_new();
    t1 = epoch_double();
    for (i = 0; i < nkeys; i++) {
        sprintf(buffer + i * 9, "%08x", i);
    }
    t2 = epoch_double();
    printf(ALIGN, "initialization", t2 - t1);

    t1 = epoch_double();
    for(i = 0; i < nkeys; i++) {
        dict_add(d, buffer + i*9, buffer +i*9);
        //printf("hash_set: key=%p, val=%p\n", buffer + i*9, buffer + i*9);
    }
    //dict_dump(d, stdout);
    t2 = epoch_double();
    printf(ALIGN, "adding", t2 - t1);

    t1 = epoch_double();
    for(i = 0; i < nkeys; i++) {
        val = dict_get(d, buffer + i*9, "UNDEF");
        if (0) {
        printf("hash_get: key=%p, val=%p\n", buffer + i*9, val);
        }
#if DEBUG>1
        printf("exp[%s] got[%s]\n", buffer+i*9, val);
        if (val && strcmp(val, buffer+i*9)) {
            printf("-> WRONG got[%s] exp[%s]\n", val, buffer+i*9);
        }
#endif
    }
    t2 = epoch_double();
    printf(ALIGN, "lookup", t2 - t1);

//    if (nkeys<100)
//        dict_dump(d, stdout);

    t1 = epoch_double();
    for(i = 0; i < nkeys; i++) {
        //printf("dict_del %d, string = %s\n", i, buffer+i*9);
        dict_del(d, buffer + i*9);
    }
    t2 = epoch_double();
    printf(ALIGN, "delete", t2 - t1);

    t1 = epoch_double();
    for(i = 0; i < nkeys; i++) {
        dict_add(d, buffer + i*9, buffer +i*9);
    }
    t2 = epoch_double();
    printf(ALIGN, "adding", t2 - t1);

    t1 = epoch_double();
    dict_free(d);
    t2 = epoch_double();
    printf(ALIGN, "free", t2 - t1);

    free(buffer);
    return 0;
}
