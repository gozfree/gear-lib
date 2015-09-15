/*****************************************************************************
 * Copyright (C) 2014-2015
 * file:    test_libvector.c
 * author:  gozfree <gozfree@163.com>
 * created: 2015-05-31 19:42
 * updated: 2015-05-31 19:42
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "libvector.h"

int main(int argc, char **argv)
{
    //struct vector *ivec = vector_init();
    VECTOR(int, ivec);

    int i;
    for (i = 0; i < 10; ++i) {
        ivec->push_back((void *)&i);
    }

    return 0;
}
