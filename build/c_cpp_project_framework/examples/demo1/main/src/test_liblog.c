/******************************************************************************
 * Copyright (C) 2014-2020 Zhifeng Gong <gozfree@163.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 ******************************************************************************/
#include <libposix.h>
#include "liblog.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "test.h"

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

static void *test(void *arg)
{
    int i;
    char *tmp = (char *)arg;
    log_init(LOG_FILE, NULL);
    for (i = 0; i < 100; i++) {
        loge("%s:%s:%d: error msg %s %d\n", __FILE__, __func__, __LINE__, tmp, i);
        logw("%s:%s:%d: warn msg %s %d\n", __FILE__, __func__, __LINE__, tmp, i);
        logi("%s:%s:%d: info msg %s %d\n", __FILE__, __func__, __LINE__, tmp, i);
        logd("%s:%s:%d: debug msg %s %d\n", __FILE__, __func__, __LINE__, tmp, i);
        logv("%s:%s:%d: verbose msg %s %d\n", __FILE__, __func__, __LINE__, tmp, i);
    }
    return NULL;
}

static void test_thread_log(void)
{
    pthread_t pid;
    pthread_create(&pid, NULL, test, "test1");
    pthread_create(&pid, NULL, test, "test2");
    pthread_create(&pid, NULL, test, "test3");
    pthread_join(pid, NULL);
}

int test_liblog()
{
    test_no_init();
    test_file_name();
    test_rsyslog();
    test_file_noname();
    test_thread_log();
    return 0;
}
