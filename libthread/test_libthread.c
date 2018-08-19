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
#include "libthread.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

void thread_print_info(struct thread *t)
{
    thread_lock(t);
    printf("========\n");
    printf("thread id = %ld\n", t->tid);
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
    printf("%s: t1->tid = %ld\n", __func__, t1->tid);
    printf("%s: t2->tid = %ld\n", __func__, t2->tid);
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
        printf("%s:%d xxx\n", __func__, __LINE__);
        sleep(1);
    }
    return 0;

}
