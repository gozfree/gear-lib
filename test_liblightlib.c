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

int main(int argc, char **argv)
{
    test_file_name();
    return 0;
}
