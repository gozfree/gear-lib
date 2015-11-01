/*****************************************************************************
 * Copyright (C) 2014-2015
 * file:    test_liblog.c
 * author:  gozfree <gozfree@163.com>
 * created: 2015-05-31 19:42
 * updated: 2015-05-31 19:42
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "liblog.h"

void test_rsyslog()
{
    int i;
    char tmp[32] = "abcd";
    log_init(LOG_RSYSLOG, "test_log");
    for (i = 0; i < 10; i++) {
        loge("test rsyslog\n");
        logw("debug msg %d, %s\n", i, tmp);
        logd("debug msg %d, %s\n", i, tmp);
        logi("debug msg %d, %s\n", i, tmp);
        logv("debug msg %d, %s\n", i, tmp);
    }
    log_deinit();
}

void test_file_name()
{
    int i;
    log_init(LOG_FILE, "tmp/foo.log");
    for (i = 0; i < 1; i++) {
        loge("debug msg\n");
        logw("debug msg\n");
        logi("debug msg\n");
        logd("xxxdebug msg\n");
        logv("debug msg\n");
    }
}

void test_file_noname()
{
    int i;
    log_init(LOG_FILE, NULL);
    for (i = 0; i < 400000; i++) {
        loge("debug msg i = %d\n", i);
        logw("debug msg\n");
        logi("debug msg\n");
        logd("debug msg\n");
        logd("debug msg\n");
    }
}

void test()
{
    int i;
    log_init(LOG_STDERR, NULL);
    for (i = 0; i < 100; i++) {
        loge("%s:%s:%d: debug msg\n", __FILE__, __func__, __LINE__);
        logw("%s:%s:%d: debug msg\n", __FILE__, __func__, __LINE__);
        logi("%s:%s:%d: debug msg\n", __FILE__, __func__, __LINE__);
        logd("%s:%s:%d: debug msg\n", __FILE__, __func__, __LINE__);
        logv("%s:%s:%d: debug msg\n", __FILE__, __func__, __LINE__);
    }
}
#if 0
static void *test2()
{
    int i;
    log_init(LOG_STDERR, NULL);
    for (i = 0; i < 1; i++) {
        loge("%s:%s:%d: debug msg\n", __FILE__, __func__, __LINE__);
        logw("%s:%s:%d: debug msg\n", __FILE__, __func__, __LINE__);
        logi("%s:%s:%d: debug msg\n", __FILE__, __func__, __LINE__);
        logd("%s:%s:%d: debug msg\n", __FILE__, __func__, __LINE__);
        logv("%s:%s:%d: debug msg\n", __FILE__, __func__, __LINE__);
    }
    return NULL;
}
#endif

int main(int argc, char **argv)
{
//    pthread_t pid;
    test_rsyslog();
#if 0
    log_init(LOG_STDERR, NULL);
    log_set_split_size(1*1024*1024);
    test_file_name();
    pthread_create(&pid, NULL, test2, NULL);

#if 0
    test_file_noname();
    test();
    pthread_create(&pid, NULL, test2, NULL);

    test_rsyslog();
#endif
    log_deinit();
    pthread_join(pid, NULL);
#endif
    return 0;
}
