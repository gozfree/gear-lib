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
#include "libthread.h"
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
