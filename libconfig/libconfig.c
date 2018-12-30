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
#include "libconfig.h"
#include <libmacro.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>


extern struct config_ops ini_ops;
extern struct config_ops json_ops;
extern struct config_ops lua_ops;

struct config *g_config = NULL;

struct config_ops_list {
    char suffix[32];
    struct config_ops *ops;
};

static struct config_ops_list conf_ops_list[] = {
#ifdef ENABLE_LUA
    {"lua", &lua_ops},
#endif
    {"json", &json_ops},
    {"ini", &ini_ops}
};

static char *get_file_suffix(const char *name)
{
    int point = '.';
    char *tmp = strrchr((char *)name, point);
    if (tmp) {
        return tmp+1;
    }
    return NULL;
}

static struct config_ops *find_backend(const char *name)
{
    if (!name) {
        printf("config name can not be NULL\n");
        return NULL;
    }
    int i = 0;
    int max_list = SIZEOF(conf_ops_list);
    char *suffix = get_file_suffix(name);
    if (!suffix) {
        printf("there is no suffix in config name\n");
        return NULL;
    }
    for (i = 0; i < max_list; i++) {
        if (!strcasecmp(conf_ops_list[i].suffix, suffix)) {
            break;
        }
    }
    if (i == max_list) {
        printf("the %s file is not supported\n", suffix);
        return NULL;
    }
    return conf_ops_list[i].ops;
}

struct config *conf_load(const char *name)
{
    struct config_ops *ops = find_backend(name);
    if (!ops) {
        printf("can not find valid config backend\n");
        return NULL;
    }
    struct config *c = CALLOC(1, struct config);
    if (!c) {
        printf("malloc failed!\n");
        return NULL;
    }
    c->ops = ops;
    if (c->ops->load) {
        if (-1 == c->ops->load(c, name)) {
            free(c);
            return NULL;
        }
    }
    g_config = c;
    return c;
}

int conf_set(struct config *c, const char *key, const char *val)
{
    if (!c || !c->ops->set_string)
        return -1;
    return c->ops->set_string(c, key, val, NULL);
}

void conf_del(struct config *c, const char *key)
{
    if (!c || !c->ops->del)
        return;
    return c->ops->del(c, key);
}

void conf_dump(struct config *c)
{
    if (!c || !c->ops->dump)
        return;
    return c->ops->dump(c, stderr);
}

int conf_save(struct config *c)
{
    if (!c || !c->ops->save)
        return -1;
    return c->ops->save(c);
}

void conf_dump_to_file(FILE *f, struct config *c)
{
    if (!c || !c->ops->dump)
        return;
    return c->ops->dump(c, f);
}

void conf_unload(struct config *c)
{
    if (c && c->ops->unload) {
        c->ops->unload(c);
    }
    free(c);
}
