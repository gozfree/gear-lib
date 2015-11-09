/******************************************************************************
 * Copyright (C) 2014-2015
 * file:    libdlmod.c
 * author:  gozfree <gozfree@163.com>
 * created: 2015-11-09 18:51
 * updated: 2015-11-09 18:51
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <liblog.h>

void *dl_load(const char *path)
{
    void *handle = dlopen(path, RTLD_NOW);
    if (!handle) {
        loge("dlopen %s failed: %s", path, dlerror());
        dlclose(handle);
        return NULL;
    }
    return handle;
}

void dl_unload(void *handle)
{
    if (handle) {
      dlclose(handle);
    }
}
