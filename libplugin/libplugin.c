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
#include "libplugin.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <libmacro.h>

struct plugin_manager *plugin_manager_create()
{
    struct plugin_manager *pm = CALLOC(1, struct plugin_manager);
    if (!pm) {
        printf("malloc failed!\n");
        return NULL;
    }
    INIT_LIST_HEAD(&pm->plugins);
    return pm;
}

void plugin_manager_destroy(struct plugin_manager *pm)
{
    if (!pm) {
        return;
    }
}

static void *plugin_get_func(struct plugin *p, const char *name)
{
    if (!p || !name) {
        return NULL;
    }
    struct plugin *q = dlsym(p->handle, name);
    if (!q) {
        printf("dlsym failed:%s\n", dlerror());
        return NULL;
    }
    return q;
}


struct plugin *plugin_lookup(struct plugin_manager *pm, const char *name)
{
    if (!pm || !name)
        return NULL;

    struct list_head* head = NULL;
    struct plugin* p = NULL;

    list_for_each(head, &pm->plugins) {
        p = list_entry(head, struct plugin, entry);
        if (0 == strcmp(p->name, name)) {
            return plugin_get_func(p, name);
        }
    }

    return NULL;
}

struct plugin *plugin_load(struct plugin_manager *pm, const char *path, const char *name)
{
    struct plugin *sym = NULL;
    struct plugin *p = NULL;
    void *handle = dlopen(path, RTLD_LAZY);
    if (!handle) {
        printf("dlopen failed: %s\n", dlerror());
        goto failed;
    }
    p = plugin_lookup(pm, name);
    if (p) {
        printf("plugin %s has already loaded!\n", name);
        return p;
    }
    sym = dlsym(handle, name);
    if (!sym) {
        printf("incompatible plugin, dlsym failed: %s\n", dlerror());
        goto failed;
    }
    p = CALLOC(1, struct plugin);
    if (!p) {
        goto failed;
    }
    p->handle = handle;
    p->name = strdup(name);
    p->path = strdup(path);
    list_add(&p->entry, &pm->plugins);
    return p;

failed:
    if (handle) dlclose(handle);
    if (p) {
        free(p->name);
        free(p->path);
        free(p);
    }
    return NULL;
}

void plugin_unload(struct plugin_manager *pm, const char *name)
{
    struct plugin *p, *tmp;
    list_for_each_entry_safe(p, tmp, &pm->plugins, entry) {
        dlclose(p->handle);
        list_del(&p->entry);
        free(p->name);
        free(p->path);
        free(p);
    }
}

struct plugin *plugin_reload(struct plugin_manager *pm, const char *path, const char *name)
{
    plugin_unload(pm, name);
    return plugin_load(pm, path, name);
}
