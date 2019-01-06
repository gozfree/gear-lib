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
#include "libworkq.h"
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/sysinfo.h>
#include <libmacro.h>

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
