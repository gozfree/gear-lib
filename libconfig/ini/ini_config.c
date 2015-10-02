/******************************************************************************
 * Copyright (C) 2014-2015
 * file:    ini_config.c
 * author:  gozfree <gozfree@163.com>
 * created: 2015-09-30 00:42
 * updated: 2015-09-30 00:42
 ******************************************************************************/
#include <libgzf.h>
#include "../libconfig.h"
#include "iniparser.h"

static struct config *load(const char *name)
{
    struct config *c = CALLOC(1, struct config);
    dictionary *ini = iniparser_load(name);
    c->instance = (void *)ini;
    return c;
}

static char *get_string(struct config *conf, const char *key)
{
    dictionary *ini = (dictionary *)conf->instance;
    return iniparser_getstring(ini, key, NULL);
}

static int get_int(struct config *conf, const char *key)
{
    dictionary *ini = (dictionary *)conf->instance;
    return iniparser_getint(ini, key, -1);
}

static double get_double(struct config *conf, const char *key)
{
    dictionary *ini = (dictionary *)conf->instance;
    return iniparser_getdouble(ini, key, -1.0);
}

static int get_boolean(struct config *conf, const char *key)
{
    dictionary *ini = (dictionary *)conf->instance;
    return iniparser_getboolean(ini, key, -1);
}

static void unload(struct config *conf)
{
    dictionary *ini = (dictionary *)conf->instance;
    iniparser_freedict(ini);
    free(conf);
}

static void dump(struct config *conf)
{
    dictionary *ini = (dictionary *)conf->instance;
    iniparser_dump(ini, stderr);
}

const struct config_ops ini_ops = {
    load,
    get_string,
    get_int,
    get_double,
    get_boolean,
    dump,
    unload,
};
