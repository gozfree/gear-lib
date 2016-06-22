/******************************************************************************
 * Copyright (C) 2014-2015
 * file:    test_liblock.c
 * author:  gozfree <gozfree@163.com>
 * created: 2016-06-22 14:09:11
 * updated: 2016-06-22 14:09:11
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include "liblock.h"

static spin_lock_t *spin = NULL;
static mutex_lock_t *mutex = NULL;

static int64_t value = 0;

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
  int n = 100;
  int c = *(int *)arg;
  for (uint64_t i = 0; i < n; ++ i) {
    if (c) {
      mutex_lock(mutex);
      ++ value;
      mutex_unlock(mutex);
    } else {
      mutex_lock(mutex);
      -- value;
      mutex_unlock(mutex);
    }
  }
  return NULL;
}
static void *print_spin_lock(void *arg)
{
  int n = 100;
  int c = *(int *)arg;
  for (uint64_t i = 0; i < n; ++ i) {
    if (c) {
      spin_lock(spin);
      ++ value;
      spin_unlock(spin);
    } else {
      spin_lock(spin);
      -- value;
      spin_unlock(spin);
    }
  }
  return NULL;
}

int main(int argc, char **argv)
{
    usage(argc, argv);
    int flag;
    pthread_t tid1, tid2;
    struct timeval start;
    struct timeval end;
    uint64_t times = strtoul((const char*)argv[2], (char**)NULL, 10);
    value = times;
    if (!strcmp(argv[1], "spin")) {
        gettimeofday(&start, NULL);
        spin = spin_init();
        flag = 0;
        pthread_create(&tid1, NULL, print_spin_lock, (void *)&flag);
        flag = 1;
        pthread_create(&tid2, NULL, print_spin_lock, (void *)&flag);
    } else if (!strcmp(argv[1], "mutex")) {
        gettimeofday(&start, NULL);
        mutex = mutex_init();
        flag = 0;
        pthread_create(&tid1, NULL, print_mutex_lock, (void *)&flag);
        flag = 1;
        pthread_create(&tid2, NULL, print_mutex_lock, (void *)&flag);
    }

    if (tid1 && tid2) {
      uint64_t startUs = 0;
      uint64_t endUs = 0;
      pthread_join(tid1, NULL);
      pthread_join(tid2, NULL);
      gettimeofday(&end, NULL);
      startUs = start.tv_sec * 1000000 + start.tv_usec;
      endUs = end.tv_sec * 1000000 + end.tv_usec;
      fprintf(stdout, "Value is %zu, Used %zuus:%zuus\n",
              value, (endUs - startUs) / 1000000, (endUs - startUs) % 1000000);
    }
    if (mutex) {
      mutex_deinit(mutex);
    }
    if (spin) {
      spin_deinit(spin);
    }

    return 0;
}
