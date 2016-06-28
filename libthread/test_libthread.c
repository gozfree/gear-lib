/*****************************************************************************
 * Copyright (C) 2014-2015
 * file:    test_libthread.c
 * author:  gozfree <gozfree@163.com>
 * created: 2015-08-16 00:38
 * updated: 2016-01-03 15:38
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

#include "libthread.h"

void thread_print_info(struct thread *t)
{
    thread_mutex_lock(t);
    printf("========\n");
    printf("thread name = %s\n", t->name);
    printf("thread id = %ld\n", t->tid);
    printf("========\n");
    thread_mutex_unlock(t);
}

static void *thread(struct thread *t, void *arg)
{
    thread_mutex_lock(t);
    thread_cond_wait(t, -1);
    thread_mutex_unlock(t);
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
    thread_mutex_unlock(t1);
    thread_destroy(t1);
    thread_destroy(t2);
    thread_destroy(t3);
}

int main(int argc, char **argv)
{
    foo();
    while (1) {
        sleep(1);
    }
    return 0;

}
