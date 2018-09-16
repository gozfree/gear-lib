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

struct plugin_info {
    char *path;
    char *name;
};

struct plugin_info p_info[] = {
    {"./modules/plugin_log.so", "plugin_log"},
    {"./modules/plugin_ipc.so", "plugin_ipc"},
    {"./modules/plugin_skt.so", "plugin_skt"},
};

#define SIZEOF(array)       (sizeof(array)/sizeof(array[0]))
static struct plugin_manager *pm = NULL;
void init()
{
    int i = 0;
    struct plugin *p = NULL;
    pm = plugin_manager_create();
    if (!pm) {
        printf("plugin_manager_create failed!\n");
        return;
    }
    for (i = 0; i < SIZEOF(p_info); i++) {
        p = plugin_load(pm, p_info[i].path, p_info[i].name);
        if (!p) {
            printf("plugin_load failed!\n");
            return;
        }
    }
}

void deinit()
{
    int i = 0;
    for (i = 0; i < SIZEOF(p_info); i++) {
        plugin_unload(pm, p_info[i].name);
    }
    plugin_manager_destroy(pm);
}

void foo()
{
    char *name = "plugin_ipc";
    struct plugin *p = plugin_lookup(pm, name);
    if (!p) {
        printf("plugin_lookup %s failed!\n", name);
        return;
    }
    p->open(NULL);
    printf("name=%s, version=%d,%d,%d\n", p->name, p->version.major, p->version.minor, p->version.patch);
}

int main(int argc, char **argv)
{
    init();
    foo();
    deinit();
    return 0;
}
