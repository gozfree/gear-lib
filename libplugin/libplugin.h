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
#ifndef LIBPLUGIN_H
#define LIBPLUGIN_H

#include <libmacro.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

struct version {
    int major;
    int minor;
    int patch;
};

struct plugin {
    char *name;
    char *path;
    struct version version;

    void *(*open)(void *arg);
    void (*close)(void *arg);
    void *(*call)(void *arg0, ...);

    void *handle;
    struct list_head entry;
};

struct plugin_manager {
    struct list_head plugins;
};

struct plugin_manager *plugin_manager_create();
void plugin_manager_destroy(struct plugin_manager *);

struct plugin *plugin_lookup(struct plugin_manager *pm, const char *name);
struct plugin *plugin_load(struct plugin_manager *pm, const char *path, const char *name);
void plugin_unload(struct plugin_manager *pm, const char *name);
struct plugin *plugin_reload(struct plugin_manager *pm, const char *path, const char *name);

#ifdef __cplusplus
}
#endif
#endif
