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
#include "libworkq.h"
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/sysinfo.h>
#include <gear-lib/libmacro.h>

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
