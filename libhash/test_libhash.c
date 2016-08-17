/******************************************************************************
 * Copyright (C) 2014-2015
 * file:    test_libhash.c
 * author:  gozfree <gozfree@163.com>
 * created: 2015-04-29 00:45
 * updated: 2015-04-29 00:45
 ******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include "libhash.h"

#define ALIGN   "%15s: %6.4f\n"
#define NKEYS   1024*1024
//#define NKEYS   10240

static double epoch_double(void)
{
    struct timeval t;
    gettimeofday(&t, NULL);
    return t.tv_sec + (t.tv_usec * 1.0) / 1000000.0;
}

int main(int argc, char * argv[])
{
    struct hash * d ;
    double t1, t2 ;
    int i ;
    int nkeys ;
    char * buffer ;
    char * val ;

    nkeys = (argc>1) ? (int)atoi(argv[1]) : NKEYS ;
    printf("%15s: %d\n", "values", nkeys);
    buffer = (char *)malloc(9 * nkeys);

    d = hash_create(2097152);
    t1 = epoch_double();
    for(i = 0; i < nkeys; i++) {
        sprintf(buffer + i * 9, "%08x", i);
    }
    t2 = epoch_double();
    printf(ALIGN, "initialization", t2 - t1);

    t1 = epoch_double();
    for(i = 0; i < nkeys; i++) {
        hash_set(d, buffer + i*9, buffer +i*9);
        //printf("hash_set: key=%p, val=%p\n", buffer + i*9, buffer + i*9);
    }
    t2 = epoch_double();
    printf(ALIGN, "adding", t2 - t1);

    t1 = epoch_double();
    for(i = 0; i < nkeys; i++) {
        val = (char *)hash_get(d, buffer + i*9);
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
        //dict_dump(d, stdout);

    t1 = epoch_double();
    for(i = 0; i < nkeys; i++) {
        hash_del(d, buffer + i*9);
    }
    t2 = epoch_double();
    printf(ALIGN, "delete", t2 - t1);

    t1 = epoch_double();
    for(i = 0; i < nkeys; i++) {
        hash_set(d, buffer + i*9, buffer +i*9);
    }
    t2 = epoch_double();
    printf(ALIGN, "adding", t2 - t1);

    t1 = epoch_double();
    hash_destroy(d);
    t2 = epoch_double();
    printf(ALIGN, "free", t2 - t1);

    free(buffer);
    return 0 ;

}
