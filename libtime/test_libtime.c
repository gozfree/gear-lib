/******************************************************************************
 * Copyright (C) 2014-2015
 * file:    test_libtime.c
 * author:  gozfree <gozfree@163.com>
 * created: 2016-05-21 17:28:20
 * updated: 2016-05-21 17:28:20
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include "libtime.h"

int main(int argc, char **argv)
{
    char time[32];
    time_get_string(time, sizeof(time));
    printf("time = %s\n", time);
    return 0;
}
