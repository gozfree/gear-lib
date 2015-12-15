/******************************************************************************
 * Copyright (C) 2014-2015
 * file:    json_config.c
 * author:  gozfree <gozfree@163.com>
 * created: 2015-12-15 23:27
 * updated: 2015-12-15 23:27
 ******************************************************************************/
#include <jansson.h>
#include <libgzf.h>
#include "../libconfig.h"

static struct config *load(const char *name)
{
    json_error_t error;
    json_t *json = json_load_file(name, 0, &error);
    if (!json) {
        printf("json_load_file %s failed!\n", name);
        return NULL;
    }
    struct config *c = CALLOC(1, struct config);
    c->instance = (void *)json;
    return c;
}

static int set_string(struct config *conf, const char *key, const char *val)
{
    return 0;
}

static char *get_string(struct config *conf, const char *key)
{
    json_t *json = (json_t *)conf->instance;
    json_t *jstring = json_object_get(json, key);
    return (char *)json_string_value(jstring);
}

static int get_int(struct config *conf, const char *key)
{
    json_t *json = (json_t *)conf->instance;
    json_t *jint = json_object_get(json, key);
    return (int)json_integer_value(jint);
}

static double get_double(struct config *conf, const char *key)
{
    json_t *json = (json_t *)conf->instance;
    json_t *jdouble = json_object_get(json, key);
    return (double)json_real_value(jdouble);
}

static int get_boolean(struct config *conf, const char *key)
{
    json_t *json = (json_t *)conf->instance;
    json_t *jbool = json_object_get(json, key);
    return (int)json_integer_value(jbool);
}

static void unload(struct config *conf)
{
    json_t *json = (json_t *)conf->instance;
    json_decref(json);
    free(conf);
}

static void dump(struct config *conf, FILE *f)
{

}

const struct config_ops json_ops = {
    load,
    set_string,
    get_string,
    get_int,
    get_double,
    get_boolean,
    dump,
    unload,
};
