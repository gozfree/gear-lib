/******************************************************************************
 * Copyright (C) 2014-2015
 * file:    libdock.h
 * author:  gozfree <gozfree@163.com>
 * created: 2015-12-17 20:26:45
 * updated: 2015-12-17 20:26:45
 *****************************************************************************/
#ifndef LIBDOCK_H
#define LIBDOCK_H

#include <libdict.h>
#include <libthread.h>
#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************

cap        |_|_|_|   |_|_|_|  |_|_|_|
plugin        ||        ||       ||
dock        ========================

 *****************************************************************************/
struct dock {
    int running;
    struct thread *thread;
    dict *plug_dict;

};

struct dock *dock_init();
void dock_deinit(struct dock *dock);


int dock_plug_add(struct dock *dock, const char *mod, const char *path);
int dock_plug_del(struct dock *dock, const char *mod);
struct dl_handle *dock_plug_get(struct dock *dock, const char *mod);
int dock_plug_mod(struct dock *dock, const char *so_name);



#ifdef __cplusplus
}
#endif
#endif
