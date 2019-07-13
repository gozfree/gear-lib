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
#include "iniparser.h"
#include "config_util.h"
#include <stdarg.h>

static int ini_load(struct config *c, const char *name)
{
    dictionary *ini = iniparser_load(name);
    if (!ini) {
        printf("iniparser_load %s failed!\n", name);
        return -1;
    }
    c->priv = (void *)ini;
    strncpy(c->path, name, sizeof(c->path));
    return 0;
}

static int ini_set_string(struct config *c, ...)
{
    dictionary *ini = (dictionary *)c->priv;
    struct int_charp *type_list = NULL;
    struct int_charp mix;
    int cnt = 0;
    va_list ap;

    va_start(ap, c);
    va_arg_type(ap, mix);
    while (mix.type != TYPE_EMPTY) {//last argument must be NULL
        cnt++;
        type_list = (struct int_charp *)realloc(type_list, cnt*sizeof(struct int_charp));
        memcpy(&type_list[cnt-1], &mix, sizeof(mix));
        va_arg_type(ap, mix);
    }
    va_end(ap);

    iniparser_set(ini, type_list[cnt-2].cval, type_list[cnt-1].cval);
    free(type_list);
    return 0;
}

static char *ini_get_string(struct config *c, ...)
{
    dictionary *ini = (dictionary *)c->priv;
    struct int_charp *type_list = NULL;
    struct int_charp mix;
    int cnt = 0;
    char *ret = NULL;
    va_list ap;

    va_start(ap, c);
    va_arg_type(ap, mix);
    while (mix.type != TYPE_EMPTY) {//last argument must be NULL
        cnt++;
        type_list = (struct int_charp *)realloc(type_list, cnt*sizeof(struct int_charp));
        memcpy(&type_list[cnt-1], &mix, sizeof(mix));
        va_arg_type(ap, mix);
    }
    va_end(ap);

    ret = iniparser_getstring(ini, type_list[cnt-1].cval, NULL);
    free(type_list);
    return ret;
}

static int ini_get_int(struct config *c, ...)
{
    dictionary *ini = (dictionary *)c->priv;
    struct int_charp *type_list = NULL;
    struct int_charp mix;
    int cnt = 0;
    int ret = 0;
    va_list ap;

    va_start(ap, c);
    va_arg_type(ap, mix);
    while (mix.type != TYPE_EMPTY) {//last argument must be NULL
        cnt++;
        type_list = (struct int_charp *)realloc(type_list, cnt*sizeof(struct int_charp));
        memcpy(&type_list[cnt-1], &mix, sizeof(mix));
        va_arg_type(ap, mix);
    }
    va_end(ap);

    ret = iniparser_getint(ini, type_list[cnt-1].cval, -1);
    free(type_list);

    return ret;
}

static double ini_get_double(struct config *c, ...)
{
    dictionary *ini = (dictionary *)c->priv;
    struct int_charp *type_list = NULL;
    struct int_charp mix;
    int cnt = 0;
    double ret = 0.0;
    va_list ap;

    va_start(ap, c);
    va_arg_type(ap, mix);
    while (mix.type != TYPE_EMPTY) {//last argument must be NULL
        cnt++;
        type_list = (struct int_charp *)realloc(type_list, cnt*sizeof(struct int_charp));
        memcpy(&type_list[cnt-1], &mix, sizeof(mix));
        va_arg_type(ap, mix);
    }
    va_end(ap);

    ret = iniparser_getdouble(ini, type_list[cnt-1].cval, -1.0);
    free(type_list);
    return ret;
}

static int ini_get_boolean(struct config *c, ...)
{
    dictionary *ini = (dictionary *)c->priv;
    struct int_charp *type_list = NULL;
    struct int_charp mix;
    int cnt = 0;
    int ret = 0;
    va_list ap;

    va_start(ap, c);
    va_arg_type(ap, mix);
    while (mix.type != TYPE_EMPTY) {//last argument must be NULL
        cnt++;
        type_list = (struct int_charp *)realloc(type_list, cnt*sizeof(struct int_charp));
        memcpy(&type_list[cnt-1], &mix, sizeof(mix));
        va_arg_type(ap, mix);
    }
    va_end(ap);

    ret = iniparser_getboolean(ini, type_list[cnt-1].cval, -1);
    free(type_list);
    return ret;
}

static void ini_del(struct config *c, const char *key)
{
    dictionary *ini = (dictionary *)c->priv;
    iniparser_unset(ini, key);
}

static void ini_unload(struct config *c)
{
    dictionary *ini = (dictionary *)c->priv;
    iniparser_freedict(ini);
}

static void ini_dump(struct config *c, FILE *f)
{
    dictionary *ini = (dictionary *)c->priv;
    iniparser_dump_ini(ini, f);
}

static int ini_save(struct config *c)
{
    dictionary *ini;
    FILE *f = fopen(c->path, "w+");
    if (!f) {
        return -1;
    }
    ini = (dictionary *)c->priv;
    iniparser_dump_ini(ini, f);
    fclose(f);
    return 0;
}

struct config_ops ini_ops = {
    ini_load,
    ini_set_string,
    ini_get_string,
    ini_get_int,
    ini_get_double,
    ini_get_boolean,
    NULL,
    ini_del,
    ini_dump,
    ini_save,
    ini_unload,
};
