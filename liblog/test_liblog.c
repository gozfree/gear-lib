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

static void test_no_init(void)
{
    int i;
    char tmp[32] = "abcd";
    for (i = 0; i < 10; i++) {
        loge("test rsyslog\n");
        logw("debug msg %d, %s\n", i, tmp);
        logd("debug msg %d, %s\n", i, tmp);
        logi("debug msg %d, %s\n", i, tmp);
        logv("debug msg %d, %s\n", i, tmp);
    }
    log_deinit();
}

static void test_rsyslog(void)
{
    int i;
    char tmp[32] = "abcd";
    log_deinit();
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

static void test_file_name(void)
{
    int i;
    log_init(LOG_FILE, "tmp/foo.log");
    log_set_split_size(1*1024*1024);

    for (i = 0; i < 1; i++) {
        loge("debug msg i=%d\n", i);
        logw("debug msg i=%d\n", i);
        logi("debug msg i=%d\n", i);
        logd("debug msg i=%d\n", i);
        logv("debug msg i=%d\n", i);
    }
    log_deinit();
}

static void test_file_noname(void)
{
    int i;
    log_init(LOG_FILE, NULL);
    log_set_split_size(1*1024*1024);
    for (i = 0; i < 100; i++) {
        loge("debug msg i = %d\n", i);
        logw("debug msg\n");
        logi("debug msg\n");
        logd("debug msg\n");
        logd("debug msg\n");
    }
    log_deinit();
}

static void test(void)
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
    log_deinit();
}

static void *test2(void *arg)
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

static void test_thread_log(void)
{
    pthread_t pid;
    pthread_create(&pid, NULL, test2, NULL);
    pthread_join(pid, NULL);
    test();
}

int main(int argc, char **argv)
{
    test_file_name();
    test_no_init();
    test_rsyslog();
    test_file_noname();
    test_thread_log();
    return 0;
}
