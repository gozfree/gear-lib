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
#include <libgzf.h>
#include <liblog.h>
#include "libconfig.h"


extern struct config_ops ini_ops;
extern struct config_ops json_ops;
extern struct config_ops lua_ops;

struct config *g_config = NULL;

struct config_ops_list {
    int index;
    struct config_ops *ops;
};

static struct config_ops_list conf_ops_list[] = {
    {CONFIG_LUA, &lua_ops},
    {CONFIG_JSON, &json_ops},
    {CONFIG_INI, &ini_ops}
};

struct config *conf_load(enum config_backend backend, const char *name)
{
    struct config *c = CALLOC(1, struct config);
    if (!c) {
        loge("malloc failed!\n");
        return NULL;
    }
    c->ops = conf_ops_list[backend].ops;
    if (c->ops->load) {
        c->ops->load(c, name);
    }
    g_config = c;
    return c;
}

int conf_set(struct config *c, const char *key, const char *val)
{
    if (!c->ops->set_string)
        return -1;
    return c->ops->set_string(c, key, val);
}

void conf_dump(struct config *c)
{
    if (!c->ops->dump)
        return;
    return c->ops->dump(c, stderr);
}

void conf_dump_to_file(FILE *f, struct config *c)
{
    if (!c->ops->dump)
        return;
    return c->ops->dump(c, f);
}

void conf_unload(struct config *c)
{
    if (c->ops->unload) {
        c->ops->unload(c);
    }
    free(c);
}
