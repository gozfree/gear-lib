/******************************************************************************
 * Copyright (C) 2014-2018 Zhifeng Gong <gozfree@163.com>
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
#include "liblock.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <sys/time.h>

static spin_lock_t spin;
static mutex_lock_t mutex;

static int64_t value = 0;
struct thread_arg {
    int flag;
    uint64_t count;
};


void usage(int argc, char **argv)
{
    if (argc != 3) {
        printf("Usage: %s <type> <count>\n", argv[0]);
        printf("type: spin | mutex\n");
        printf("count: 2 ~ 10\n");
        exit(0);
    }

}

static void *print_mutex_lock(void *arg)
{
    struct thread_arg *argp = (struct thread_arg *)arg;
    int c = argp->flag;
    uint64_t n = argp->count;
    uint64_t i;
    pthread_t tid = pthread_self();
    printf("c = %d\n", c);
    for (i = 0; i < n; ++ i) {
        if (c) {
            mutex_lock(&mutex);
            ++ value;
            if (!(i % 100) && 0) {
                printf("tid=%d, c=%d, value = %"PRId64"\n", (int)tid, c, value);
            }
            mutex_unlock(&mutex);
        } else {
            mutex_lock(&mutex);
            -- value;
            if (!(i % 100) && 0) {
                printf("tid=%d, c=%d, value = %"PRId64"\n", (int)tid, c, value);
            }
            mutex_unlock(&mutex);
        }
    }
    return NULL;
}
static void *print_spin_lock(void *arg)
{
    struct thread_arg *argp = (struct thread_arg *)arg;
    int c = argp->flag;
    uint64_t n = argp->count;
    uint64_t i;
    pthread_t tid = pthread_self();

    printf("c = %d\n", c);
    for (i = 0; i < n; ++ i) {
        if (c) {
            spin_lock(&spin);
            ++ value;
            if (!(i % 100) && 0) {
                printf("tid=%d, c=%d, value = %"PRId64"\n", (int)tid, c, value);
            }
            spin_unlock(&spin);
        } else {
            spin_lock(&spin);
            -- value;
            if (!(i % 100) && 0) {
                printf("tid=%d, c=%d, value = %"PRId64"\n", (int)tid, c, value);
            }
            spin_unlock(&spin);
        }
    }
    return NULL;
}

int main(int argc, char **argv)
{
    usage(argc, argv);
    pthread_t tid1, tid2;
    struct timeval start;
    struct timeval end;
    uint64_t times = strtoul((const char*)argv[2], (char**)NULL, 10);
    value = times;
    struct thread_arg arg1, arg2;
    arg1.flag = 0;
    arg1.count = times;
    arg2.flag = 1;
    arg2.count = times;
    if (!strcmp(argv[1], "spin")) {
        gettimeofday(&start, NULL);
        pthread_create(&tid1, NULL, print_spin_lock, (void *)&arg1);
        pthread_create(&tid2, NULL, print_spin_lock, (void *)&arg2);
    } else if (!strcmp(argv[1], "mutex")) {
        gettimeofday(&start, NULL);
        mutex_lock_init(&mutex);
        pthread_create(&tid1, NULL, print_mutex_lock, (void *)&arg1);
        pthread_create(&tid2, NULL, print_mutex_lock, (void *)&arg2);
    }
    printf("tid1=%d, tid2=%d\n", (int)tid1, (int)tid2);

    if (tid1 && tid2) {
        uint64_t startUs = 0;
        uint64_t endUs = 0;
        pthread_join(tid1, NULL);
        pthread_join(tid2, NULL);
        gettimeofday(&end, NULL);
        startUs = start.tv_sec * 1000000 + start.tv_usec;
        endUs = end.tv_sec * 1000000 + end.tv_usec;
        fprintf(stdout, "Value is %" PRIu64 ", Used %" PRIu64 "us:%" PRIu64 "\n",
                value, (endUs - startUs) / 1000000, (endUs - startUs) % 1000000);
    }
    mutex_lock_deinit(&mutex);

    return 0;
}
