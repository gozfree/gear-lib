/******************************************************************************
 * Copyright (C) 2014-2015
 * file:    test_liblightlib.c
 * author:  gozfree <gozfree@163.com>
 * created: 2016-07-21 15:12
 * updated: 2016-07-21 15:12
 ******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "liblightlib.h"

void test_file_name()
{
    int i;
    for (i = 0; i < 1; i++) {
        loge("debug msg i=%d\n", i);
        logw("debug msg i=%d\n", i);
        logi("debug msg i=%d\n", i);
        logd("debug msg i=%d\n", i);
        logv("debug msg i=%d\n", i);
    }
    log_deinit();
}
void foo()
{
   char buf[128] = {0};
   struct file *f = file_open("/tmp/lsusb", F_RDONLY);
   file_read(f, buf, sizeof(buf));
   printf("buf =%s", buf);
   printf("len=%zu\n", file_size("/tmp/lsusb"));
}


int main(int argc, char **argv)
{
    test_file_name();
    foo();
    return 0;
}
