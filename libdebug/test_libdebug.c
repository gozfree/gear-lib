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

static void foo(void)
{
    char *tmp = NULL;
    *tmp = 0;
    printf("xxx=%s\n", tmp);
    free(tmp);
    free(tmp);
}

static void foo2(void)
{
    foo();
}

static void foo3(void)
{
    foo2();
}

int main(int argc, char **argv)
{
    debug_backtrace_init();
    foo3();

    return 0;
}
