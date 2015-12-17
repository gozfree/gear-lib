/******************************************************************************
 * Copyright (C) 2014-2015
 * file:    test_libdlmod.c
 * author:  gozfree <gozfree@163.com>
 * created: 2015-11-09 18:52
 * updated: 2015-11-09 18:52
 ******************************************************************************/
#include <unistd.h>
#include <libgzf.h>
#include "libdlmod.h"
#include <libthread.h>

dl_thread_create _thread_create;
dl_thread_print_info _thread_print_info;
dl_thread_mutex_lock _thread_mutex_lock;
dl_thread_mutex_unlock _thread_mutex_unlock;
dl_thread_cond_wait _thread_cond_wait;


static void *thread(struct thread *t, void *arg)
{
    _thread_mutex_lock(t);
    _thread_cond_wait(t);
    _thread_mutex_unlock(t);
    return NULL;
}

void test_thread()
{
    int i;
    struct capability_desc desc;
    struct dl_handle *dl = dl_load("/usr/local/lib/libthread.so");
    dl_capability(dl, "thread", &desc);
    for (i = 0; i < desc.entry; ++i) {
        printf("cap[%d] = %s\n", i, desc.cap[i]);
    }
    _thread_create = (dl_thread_create)dl_get_func(dl, desc.cap[THREAD_CREATE]);
    _thread_print_info = (dl_thread_print_info)dl_get_func(dl, desc.cap[THREAD_PRINT_INFO]);
    _thread_mutex_lock = (dl_thread_mutex_lock)dl_get_func(dl, desc.cap[THREAD_MUTEX_LOCK]);
    _thread_mutex_unlock = (dl_thread_mutex_lock)dl_get_func(dl, desc.cap[THREAD_MUTEX_UNLOCK]);
    _thread_cond_wait = (dl_thread_cond_wait)dl_get_func(dl, desc.cap[THREAD_COND_WAIT]);

    struct thread *t1 = _thread_create("t1", thread, NULL);
    struct thread *t2 = _thread_create("t2", thread, NULL);
    struct thread *t3 = _thread_create(NULL, thread, NULL);
    printf("%s: t1->tid = %ld\n", __func__, t1->tid);
    printf("%s: t2->tid = %ld\n", __func__, t2->tid);
    _thread_print_info(t1);
    _thread_print_info(t2);
    _thread_print_info(t3);
    while (1) {
        sleep(1);
    }

    dl_unload(dl);
}

int main(int argc, char **argv)
{
    test_thread();
    return 0;
}
