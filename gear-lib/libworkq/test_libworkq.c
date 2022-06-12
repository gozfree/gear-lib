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

void test(void *arg)
{
    int *i = (int *)arg;
    usleep(100 * 1000);
    printf("%s:%d i=%03d\n", __func__, __LINE__, *i);
}


int foo()
{
    int ret;
    int i = 0;
    int array[20] = {0};
    struct workq_pool *pool = workq_pool_create();
    for (i = 0; i < 20; i++) {
        array[i] = i;
        ret = workq_pool_task_push(pool, test, &array[i]);
        if (ret < 0) {
            printf("workq_pool_task_push %d failed!\n", i);
        } else {
            printf("workq_pool_task_push %d success!\n", i);
        }
        if (i %10 == 0)
            sleep(1);
    }
    sleep(4);
    workq_pool_destroy(pool);
    return 0;
}

int main()
{
    foo();
    while (1) {
        printf("main loop\n");
        sleep(1);
    }
    return 0;
}
