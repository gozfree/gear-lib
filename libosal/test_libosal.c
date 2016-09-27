/******************************************************************************
 * Copyright (C) 2014-2015
 * file:    test_libosal.c
 * author:  gozfree <gozfree@163.com>
 * created: 2015-11-28 17:59:24
 * updated: 2015-11-28 17:59:24
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include "libosal.h"

int main(int argc, char **argv)
{
    char *cmd = "date";
    char buf[64];
    system_with_result(cmd, buf, sizeof(buf));
    printf("buf = %s\n", buf);
    return 0;
}
