/*****************************************************************************
 * Copyright (C) 2014-2015
 * file:    test_libworkq.c
 * author:  gozfree <gozfree@163.com>
 * created: 2015-07-13 02:03
 * updated: 2015-07-19 20:44
 *****************************************************************************/
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/sysinfo.h>
#include <libmacro.h>
#include "libworkq.h"

void test(void *arg)
{
    int *i = (int *)arg;
    usleep(100 * 1000);
    printf("%s:%d i=%03d\n", __func__, __LINE__, *i);
}

static struct workq *g_wq = NULL;
int foo()
{
    int i = 0;
    g_wq = wq_create();
    for (i = 0; i < 4; i++) {
        wq_task_add(g_wq, test, (void *)&i, sizeof(int));
    }
    sleep(1);
    wq_destroy(g_wq);
    return 0;
}

int foo2()
{
    int i = 0;
    wq_pool_init();
    for (i = 0; i < 10; i++) {
        printf("%s:%d i = %d\n", __func__, __LINE__, i);
        wq_pool_task_add(test, (void *)&i, sizeof(int));
    }
    sleep(1);
    wq_pool_deinit();
    return 0;
}

int main()
{
    int i = 0;
    foo2();
    while (1) {
        printf("main loop\n");
        i++;
        //wq_task_add(g_wq, test, (void *)&i, sizeof(int));
        //wq_pool_task_add(test, (void *)&i, sizeof(int));
        sleep(1);
    }
    return 0;
}
