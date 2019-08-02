/******************************************************************************
 * Copyright (C) 2014-2020 Zhifeng Gong <gozfree@163.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
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
