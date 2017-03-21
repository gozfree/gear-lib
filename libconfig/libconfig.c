/*****************************************************************************
 * Copyright (C) 2014-2015
 * file:    libconfig.c
 * author:  gozfree <gozfree@163.com>
 * created: 2015-09-29 23:51
 * updated: 2015-09-29 23:51
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <libmacro.h>
#include <liblog.h>
#include "libconfig.h"


extern struct config_ops ini_ops;
extern struct config_ops json_ops;
extern struct config_ops lua_ops;

struct config *g_config = NULL;

struct config_ops_list {
    char suffix[32];
    struct config_ops *ops;
};

static struct config_ops_list conf_ops_list[] = {
    {"lua", &lua_ops},
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
        loge("config name can not be NULL\n");
        return NULL;
    }
    int i = 0;
    int max_list = (sizeof(conf_ops_list)/sizeof(config_ops_list));
    char *suffix = get_file_suffix(name);
    if (!suffix) {
        loge("there is no suffix in config name\n");
        return NULL;
    }
    for (i = 0; i < max_list; i++) {
        if (!strcasecmp(conf_ops_list[i].suffix, suffix)) {
            break;
        }
    }
    if (i == max_list) {
        loge("the %s file is not supported\n", suffix);
        return NULL;
    }
    return conf_ops_list[i].ops;
}

struct config *conf_load(const char *name)
{
    struct config_ops *ops = find_backend(name);
    if (!ops) {
        loge("can not find valid config backend\n");
        return NULL;
    }
    struct config *c = CALLOC(1, struct config);
    if (!c) {
        loge("malloc failed!\n");
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
