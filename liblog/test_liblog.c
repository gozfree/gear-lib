/******************************************************************************
 * Copyright (C) 2014-2017 Zhifeng Gong <gozfree@163.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with libraries; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 ******************************************************************************/
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

int main(int argc, char **argv)
{
    test_no_init();
    test_file_name();
    test_rsyslog();
    test_file_noname();
    test_thread_log();
    return 0;
}
