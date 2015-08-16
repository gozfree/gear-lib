/*****************************************************************************
 * Copyright (C) 2014-2015
 * file:    test_libthread.c
 * author:  gozfree <gozfree@163.com>
 * created: 2015-08-16 00:38
 * updated: 2015-08-16 00:38
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>

#include "libthread.h"

static void *thread(struct thread *t, void *arg)
{
    printf("into thread, thread_id = %ld\n", pthread_self());
    thread_mutex_lock(t);
    thread_cond_wait(t);
    thread_mutex_unlock(t);
    return NULL;
}

void foo()
{
    struct thread *t1 = thread_create(thread, NULL);
//    struct thread *t2 = thread_create(thread, NULL);
    printf("%s: t1->tid = %ld\n", __func__, t1->tid);
//    printf("%s: t2->tid = %ld\n", __func__, t2->tid);
    
}

int main(int argc, char **argv)
{
    foo();
    while (1) {
        sleep(1);
    }
    return 0;

}
