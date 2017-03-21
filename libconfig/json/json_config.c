/******************************************************************************
 * Copyright (C) 2014-2015
 * file:    json_config.c
 * author:  gozfree <gozfree@163.com>
 * created: 2015-12-15 23:27
 * updated: 2015-12-15 23:27
 ******************************************************************************/
#include <jansson.h>
#include <libmacro.h>
#include <liblog.h>
#include "../libconfig.h"

static int js_load(struct config *c, const char *name)
{
    json_error_t error;
    json_t *json = json_load_file(name, 0, &error);
    if (!json) {
        loge("json_load_file %s failed!\n", name);
        return -1;
    }
    c->priv = (void *)json;
    return 0;
}

static char *js_get_string(struct config *c, ...)
{
    json_t *json = (json_t *)c->priv;
    json_t *jstring;

    char **key = NULL;
    char *tmp = NULL;
    char *ret = NULL;
    int cnt = 0;
    va_list ap;
    va_start(ap, c);
    tmp = va_arg(ap, char *);
    while (tmp) {//last argument must be NULL
        cnt++;
        key = (char **)realloc(key, cnt*sizeof(char**));
        key[cnt-1] = tmp;
        tmp = va_arg(ap, char *);
    }
    va_end(ap);
    switch (cnt) {
    case 1:
        jstring = json_object_get(json, key[0]);
        ret = (char *)json_string_value(jstring);
        break;
    default:
        break;
    }
    free(key);
    return ret;
}

static int js_get_int(struct config *c, ...)
{
    json_t *json = (json_t *)c->priv;
    json_t *jint;
    char **key = NULL;
    char *tmp = NULL;
    int ret = 0;
    int cnt = 0;
    va_list ap;
    va_start(ap, c);
    tmp = va_arg(ap, char *);
    while (tmp) {//last argument must be NULL
        cnt++;
        key = (char **)realloc(key, cnt*sizeof(char**));
        key[cnt-1] = tmp;
        tmp = va_arg(ap, char *);
    }
    va_end(ap);
    switch (cnt) {
    case 1:
        jint = json_object_get(json, key[0]);
        ret = (int)json_integer_value(jint);
        break;
    default:
        break;
    }
    free(key);
    return ret;
}

static double js_get_double(struct config *c, ...)
{
    json_t *json = (json_t *)c->priv;
    json_t *jdouble;
    char **key = NULL;
    char *tmp = NULL;
    double ret;
    int cnt = 0;
    va_list ap;
    va_start(ap, c);
    tmp = va_arg(ap, char *);
    while (tmp) {//last argument must be NULL
        cnt++;
        key = (char **)realloc(key, cnt*sizeof(char**));
        key[cnt-1] = tmp;
        tmp = va_arg(ap, char *);
    }
    va_end(ap);
    switch (cnt) {
    case 1:
        jdouble = json_object_get(json, key[0]);
        ret = (double)json_real_value(jdouble);
        break;
    default:
        break;
    }
    free(key);
    return ret;
}

static int js_get_boolean(struct config *c, ...)
{
    json_t *json = (json_t *)c->priv;
    json_t *jbool;
    char **key = NULL;
    char *tmp = NULL;
    int ret;
    int cnt = 0;
    va_list ap;
    va_start(ap, c);
    tmp = va_arg(ap, char *);
    while (tmp) {//last argument must be NULL
        cnt++;
        key = (char **)realloc(key, cnt*sizeof(char**));
        key[cnt-1] = tmp;
        tmp = va_arg(ap, char *);
    }
    va_end(ap);
    switch (cnt) {
    case 1:
        jbool = json_object_get(json, key[0]);
        ret = (int)json_integer_value(jbool);
        break;
    default:
        break;
    }
    free(key);
    return ret;
}

static void js_unload(struct config *c)
{
    json_t *json = (json_t *)c->priv;
    json_decref(json);
}

struct config_ops json_ops = {
    .load        = js_load,
    .set_string  = NULL,
    .get_string  = js_get_string,
    .get_int     = js_get_int,
    .get_double  = js_get_double,
    .get_boolean = js_get_boolean,
    .del         = NULL,
    .dump        = NULL,
    .save        = NULL,
    .unload      = js_unload,
};
