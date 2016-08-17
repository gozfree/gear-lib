/******************************************************************************
 * Copyright (C) 2014-2015
 * file:    test_libdock.c
 * author:  gozfree <gozfree@163.com>
 * created: 2015-12-17 20:26:45
 * updated: 2015-12-17 20:26:45
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <libmacro.h>
#include <liblog.h>
#include <libthread.h>
#include <libdlmod.h>
#include "libdock.h"

typedef struct libthread {
   int (*capability)(struct capability_desc *desc);
   struct thread *(*create)(const char *name,
                   void *(*func)(struct thread *, void *), void *arg);
   void (*destroy)(struct thread *t);
   int (*cond_wait)(struct thread *t);
   int (*cond_signal)(struct thread *t);
   int (*mutex_lock)(struct thread *t);
   int (*mutex_unlock)(struct thread *t);
   int (*sem_lock)(struct thread *t);
   int (*sem_unloock)(struct thread *t);
   int (*sem_wait)(struct thread *t, int64_t ms);
   int (*sem_signal)(struct thread *t);
   void (*print_info)(struct thread *t);
} libthread_t;

enum thread_cap {
    THREAD_CAPABILITY = 0,
    THREAD_CREATE,
    THREAD_DESTROY,
    THREAD_COND_WAIT,
    THREAD_COND_SIGNAL,
    THREAD_MUTEX_LOCK,
    THREAD_MUTEX_UNLOCK,
    THREAD_SEM_LOCK,
    THREAD_SEM_UNLOCK,
    THREAD_SEM_WAIT,
    THREAD_SEM_SIGNAL,
    THREAD_PRINT_INFO,
};


static void *thread(struct thread *t, void *arg)
{
    struct libthread *thread_if = (struct libthread *)arg;
    thread_if->mutex_lock(t);
    thread_if->cond_wait(t);
    thread_if->mutex_unlock(t);
    return NULL;
}


static int test(void)
{
    int ret = 0;
    int i;
    struct capability_desc desc;
    struct dock *dock = dock_init();
    ret = dock_plug_add(dock, "thread", "/usr/local/lib/libthread.so");
    if (ret != 0) {
        printf("dock_plug_add failed!\n");
        return -1;
    }
    struct dl_handle *dl = dock_plug_get(dock, "thread");

    ret = dl_capability(dl, "thread", &desc);
    if (ret != 0) {
        printf("dl_capability failed!\n");
        return -1;
    }
    for (i = 0; i < desc.entry; ++i) {
        printf("cap[%d] = %s\n", i, desc.cap[i]);
    }

    struct libthread *thread_if = CALLOC(1, struct libthread);
    thread_if->create = dl_get_func(dl, desc.cap[THREAD_CREATE]);
    thread_if->print_info = dl_get_func(dl, desc.cap[THREAD_PRINT_INFO]);
    thread_if->mutex_lock = dl_get_func(dl, desc.cap[THREAD_MUTEX_LOCK]);
    thread_if->mutex_unlock = dl_get_func(dl, desc.cap[THREAD_MUTEX_UNLOCK]);
    thread_if->cond_wait = dl_get_func(dl, desc.cap[THREAD_COND_WAIT]);
    if (!(thread_if->create && thread_if->print_info && thread_if->mutex_lock
       && thread_if->mutex_unlock && thread_if->cond_wait)) {
        printf("load libthread failed!\n");
        return -1;
    }

    struct thread *t1 = thread_if->create("t1", thread, thread_if);
    struct thread *t2 = thread_if->create("t2", thread, thread_if);
    struct thread *t3 = thread_if->create(NULL, thread, thread_if);
    printf("%s: t1->tid = %ld\n", __func__, t1->tid);
    printf("%s: t2->tid = %ld\n", __func__, t2->tid);
    thread_if->print_info(t1);
    thread_if->print_info(t2);
    thread_if->print_info(t3);
    while (1) {
        sleep(1);
    }

    dl_unload(dl);
    return 0;
}

int main(int argc, char **argv)
{
    test();
    //test_thread();
    return 0;
}
