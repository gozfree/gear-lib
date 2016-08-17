/******************************************************************************
 * Copyright (C) 2014-2015
 * file:    libdlmod.c
 * author:  gozfree <gozfree@163.com>
 * created: 2015-11-09 18:51
 * updated: 2015-11-09 18:51
 ******************************************************************************/
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <libmacro.h>
#include <liblog.h>
#include "libdlmod.h"

#define CAPABILITY  "capability"

typedef int (*get_capability)(struct capability_desc *desc);


struct dl_handle *dl_load(const char *path)
{
    void *handle = dlopen(path, RTLD_NOW);
    if (!handle) {
        loge("dlopen %s failed: %s", path, dlerror());
        dlclose(handle);
        return NULL;
    }
    struct dl_handle *dl = CALLOC(1, struct dl_handle);
    dl->handle = handle;
    return dl;
}

void dl_unload(struct dl_handle *dl)
{
    if (!dl) {
        return;
    }
    if (dl->handle) {
      dlclose(dl->handle);
    }
}

int dl_capability(struct dl_handle *dl, const char *mod_name,
                struct capability_desc *desc)
{
    if (!dl) {
        return -1;
    }
    char *err = NULL;
    char symbol[512];
    snprintf(symbol, sizeof(symbol), "%s_%s", mod_name, CAPABILITY);
    get_capability get_cap = (get_capability)dlsym(dl->handle, symbol);
    if (((err = dlerror()) != NULL)) {
        loge("Failed to get symbol %s: %s\n", symbol, err);
        return -1;
    }
    get_cap(desc);
    return 0;
}

void *dl_get_func(struct dl_handle *dl, const char *name)
{
    if (!dl) {
        return NULL;
    }
    void *func = dlsym(dl->handle, name);
    if (!func) {
        loge("Failed to get symbol %s: %s\n", name, dlerror());
    }
    return func;
}

void *dl_override(const char *name)
{
    void *sym = dlsym(RTLD_NEXT, name);
    if (!sym) {
        loge("Failed override symbol %s: %s\n", name, dlerror());
        abort();
    }
    return sym;
}
