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
#include <stdarg.h>

static char *file_path = NULL;

static int ini_load(struct config *c, const char *name)
{
    dictionary *ini = iniparser_load(name);
    if (!ini) {
        printf("iniparser_load %s failed!\n", name);
        return -1;
    }
    c->priv = (void *)ini;
    file_path = strdup(name);
    return 0;
}

static int ini_set_string(struct config *c, ...)
{
#if 0
    const char *key;
    const char *val;
    const char *end;
    dictionary *ini = (dictionary *)c->priv;
    return iniparser_set(ini, key, val);
#else
    return 0;
#endif
}

static char *ini_get_string(struct config *c, ...)
{
    dictionary *ini = (dictionary *)c->priv;
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
        ret = iniparser_getstring(ini, key[0], NULL);
        break;
    case 2:
        break;
    case 3:
        break;
    case 4:
        break;
    case 0:
    default:
        break;
    }
    free(key);
    return ret;
}

static int ini_get_int(struct config *c, ...)
{
    dictionary *ini = (dictionary *)c->priv;
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
        ret = iniparser_getint(ini, key[0], -1);
        break;
    default:
        break;
    }
    free(key);
    return ret;
}

static double ini_get_double(struct config *c, ...)
{
    dictionary *ini = (dictionary *)c->priv;
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
        ret = iniparser_getdouble(ini, key[0], -1.0);
        break;
    default:
        break;
    }
    free(key);
    return ret;
}

static int ini_get_boolean(struct config *c, ...)
{
    dictionary *ini = (dictionary *)c->priv;
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
        ret = iniparser_getboolean(ini, key[0], -1);
        break;
    default:
        break;
    }
    free(key);
    return ret;
}

static void ini_del(struct config *c, const char *key)
{
    dictionary *ini = (dictionary *)c->priv;
    return iniparser_unset(ini, key);
}


static void ini_unload(struct config *c)
{
    dictionary *ini = (dictionary *)c->priv;
    iniparser_freedict(ini);
    free(file_path);
}

static void ini_dump(struct config *c, FILE *f)
{
    dictionary *ini = (dictionary *)c->priv;
    iniparser_dump_ini(ini, f);
}

static int ini_save(struct config *c)
{
    FILE *f = fopen(file_path, "w+");
    if (!f) {
        return -1;
    }
    dictionary *ini = (dictionary *)c->priv;
    iniparser_dump_ini(ini, f);
    fclose(f);
    return 0;
}

struct config_ops ini_ops = {
    .load        = ini_load,
    .set_string  = ini_set_string,
    .get_string  = ini_get_string,
    .get_int     = ini_get_int,
    .get_double  = ini_get_double,
    .get_boolean = ini_get_boolean,
    .get_length  = NULL,
    .del         = ini_del,
    .dump        = ini_dump,
    .save        = ini_save,
    .unload      = ini_unload,
};
