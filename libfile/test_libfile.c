/******************************************************************************
 * Copyright (C) 2014-2015
 * file:    test_libfile.c
 * author:  gozfree <gozfree@163.com>
 * created: 2016-07-22 14:17:17
 * updated: 2016-07-22 14:17:17
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include "libfile.h"


void foo()
{
   char buf[128] = {0};
   struct file *f = file_open("/tmp/adb.log", F_RDONLY);
   file_read(f, buf, sizeof(buf));
   printf("buf =%s", buf);
}

int main(int argc, char **argv)
{
    foo();
    return 0;
}
