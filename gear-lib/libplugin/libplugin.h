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
#ifndef LIBPLUGIN_H
#define LIBPLUGIN_H

#include <gear-lib/libmacro.h>
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

/*
 * using HOOK_CALL(func, args...), prev/post functions can be hook into func
 */
#define HOOK_CALL(fn, ...)                                \
    ({                                                    \
        fn##_prev(__VA_ARGS__);                           \
        __typeof__(fn) *sym =  dlsym(RTLD_NEXT, #fn);     \
        if (!sym) {return NULL;}                          \
        sym(__VA_ARGS__);                                 \
        fn##_post(__VA_ARGS__);                           \
    })

/*
 * using CALL(fn, args...), you need override api
 */
#define CALL(fn, ...)                                     \
    ({__typeof__(fn) *sym = (__typeof__(fn) *)            \
                            dlsym(RTLD_NEXT, #fn);        \
     sym(__VA_ARGS__);})


#ifdef __cplusplus
}
#endif
#endif
