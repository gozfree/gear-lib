/*****************************************************************************
 * Copyright (C) 2014-2015
 * file:    libglog.c
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
#include "libconfig.h"


extern const struct config_ops ini_ops;

static const struct config_ops *conf_ops[] = {
    &ini_ops,
    NULL
};

struct config *conf_load(const char *name)
{
    return conf_ops[0]->load(name);
}

int conf_get_int(struct config *conf, const char *key)
{
    return conf_ops[0]->get_int(conf, key);
}

char *conf_get_string(struct config *conf, const char *key)
{
    return conf_ops[0]->get_string(conf, key);
}

double conf_get_double(struct config *conf, const char *key)
{
    return conf_ops[0]->get_double(conf, key);
}

int conf_get_boolean(struct config *conf, const char *key)
{
    return conf_ops[0]->get_boolean(conf, key);
}

void conf_dump(struct config *conf)
{
    return conf_ops[0]->dump(conf);
}

void conf_unload(struct config *conf)
{
    return conf_ops[0]->unload(conf);
}
