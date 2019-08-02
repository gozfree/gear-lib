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
#include <errno.h>
#if defined (__linux__) || defined (__CYGWIN__)
#include <unistd.h>
#endif

void thread_print_info(struct thread *t)
{
    thread_lock(t);
    printf("========\n");
    printf("thread id = %ld\n", (long)t->tid);
    printf("========\n");
    thread_unlock(t);
}

static void *thread(struct thread *t, void *arg)
{
    thread_lock(t);
    thread_wait(t, -1);
    thread_unlock(t);
    return NULL;
}

void foo()
{
    struct thread *t1 = thread_create(thread, NULL);
    struct thread *t2 = thread_create(thread, NULL);
    struct thread *t3 = thread_create(thread, NULL);
    printf("%s: t1->tid = %ld\n", __func__, (long)t1->tid);
    printf("%s: t2->tid = %ld\n", __func__, (long)t2->tid);
    thread_print_info(t1);
    thread_print_info(t2);
    thread_print_info(t3);
    sleep(1);
    thread_destroy(t1);
    thread_destroy(t2);
    thread_destroy(t3);
}

void foo2()
{
    struct thread *t1 = thread_create(thread, NULL);
    thread_info(t1);
    thread_destroy(t1);
}

int main(int argc, char **argv)
{
    foo();
    foo2();
    while (1) {
        //printf("%s:%d xxx\n", __func__, __LINE__);
        sleep(1);
    }
    return 0;

}
