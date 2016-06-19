/******************************************************************************
 * Copyright (C) 2014-2015
 * file:    test_libdebug.c
 * author:  gozfree <gozfree@163.com>
 * created: 2016-06-17 16:53:13
 * updated: 2016-06-17 16:53:13
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include "libdebug.h"

void foo()
{
    char *tmp = NULL;
    debug_register_backtrace();
    *tmp = 0;
    printf("xxx=%s\n", tmp);
    free(tmp);
    free(tmp);
}

int main(int argc, char **argv)
{
    foo();

    return 0;
}
