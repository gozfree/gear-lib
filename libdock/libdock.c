/******************************************************************************
 * Copyright (C) 2014-2015
 * file:    libdock.c
 * author:  gozfree <gozfree@163.com>
 * created: 2015-12-17 20:26:45
 * updated: 2015-12-17 20:26:45
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <libmacro.h>
#include <liblog.h>
#include <libdict.h>
#include <libthread.h>
#include <libdlmod.h>
#include "libdock.h"


int dock_plug_add(struct dock *dock, const char *mod, const char *path)
{
    struct dl_handle *dl = dl_load(path);
    struct capability_desc desc;
    if (-1 == dl_capability(dl, mod, &desc)) {
        loge("dl_capability failed!\n");
        return -1;
    }
    dict_add(dock->plug_dict, (char *)mod, (char *)dl);
    thread_sem_signal(dock->thread);
    return 0;
}

int dock_plug_del(struct dock *dock, const char *mod)
{
    return dict_del(dock->plug_dict, (char *)mod);
}

struct dl_handle *dock_plug_get(struct dock *dock, const char *mod)
{
    struct dl_handle *dl = (struct dl_handle *)dict_get(dock->plug_dict,
                    (char *)mod, NULL);
    return dl;
}


static void *dock_thread(struct thread *t, void *arg)
{
    struct dock *dock = (struct dock *)arg;
    dock->running = 1;
    while (dock->running) {
        thread_sem_wait(t, -1);
        loge("xxx\n");
    }
    return NULL;
}


struct dock *dock_init(void)
{
    struct dock *dock = CALLOC(1, struct dock);
    dock->plug_dict = dict_new();
    dock->thread = thread_create(dock_thread, dock);

    return dock;
}

void dock_deinit(struct dock *dock)
{
    dock->running = 0;
    thread_sem_signal(dock->thread);
    thread_destroy(dock->thread);
    free(dock);
}
