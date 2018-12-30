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
#include <libmacro.h>
#include "../libconfig.h"
#include "cJSON.h"
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#define POINTER_ADDR 0x10000000

#define TYPE_EMPTY   0
#define TYPE_INT     1
#define TYPE_CHARP   2

struct int_charp {
    int type;
    union {
        int ival;
        char *cval;
    };
};

static char *read_file(const char *filename)
{
    FILE *file = NULL;
    long length = 0;
    char *content = NULL;
    size_t read_chars = 0;

    file = fopen(filename, "rb");
    if (file == NULL) {
        goto cleanup;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        goto cleanup;
    }
    length = ftell(file);
    if (length < 0) {
        goto cleanup;
    }
    if (fseek(file, 0, SEEK_SET) != 0) {
        goto cleanup;
    }

    content = (char*)malloc((size_t)length + sizeof(""));
    if (content == NULL) {
        goto cleanup;
    }

    read_chars = fread(content, sizeof(char), (size_t)length, file);
    if ((long)read_chars != length) {
        free(content);
        content = NULL;
        goto cleanup;
    }
    content[read_chars] = '\0';

cleanup:
    if (file != NULL) {
        fclose(file);
    }
    return content;
}

static int js_load(struct config *c, const char *name)
{
    char *buf = read_file(name);
    if (!buf) {
        printf("read_file %s failed!\n", name);
        return -1;
    }
    cJSON *json = cJSON_Parse(buf);
    if (!json) {
        printf("cJSON_Parse failed!\n");
        free(buf);
        return -1;
    }
    free(buf);
    c->priv = (void *)json;
    return 0;
}

/*
 * va_arg_type can get value from ap ignore type
 * if the type is char *, it must be pointer of memory in higher address
 * when the type is int, it always smaller than pointer addr
 */
#define va_arg_type(ap, mix)                        \
    do {                                            \
        char *__tmp = va_arg(ap, char *);           \
        if (!__tmp) {                               \
            mix.type = TYPE_EMPTY;                  \
            mix.cval = NULL;                        \
        } else if (__tmp < (char *)POINTER_ADDR) {  \
            mix.type = TYPE_INT;                    \
            mix.ival = *(int *)&__tmp;              \
        } else {                                    \
            mix.type = TYPE_CHARP;                  \
            mix.cval = __tmp;                       \
        }                                           \
    } while (0)

static int js_set_string(struct config *c, ...)
{
    cJSON *json = (cJSON *)c->priv;
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

    for (int i = 0; i < cnt-1; i++) {
        switch (type_list[i].type) {
        case TYPE_INT:
            json = cJSON_GetArrayItem(json, type_list[i].ival-1);
            break;
        case TYPE_CHARP:
            json = cJSON_GetObjectItem(json, type_list[i].cval);
            break;
        default:
            break;
        }
    }
    cJSON_AddItemToObject(json, type_list[cnt-2].cval, cJSON_CreateString(type_list[cnt-1].cval));
    free(type_list);
    return 0;
}

static char *js_get_string(struct config *c, ...)
{
    cJSON *json = (cJSON *)c->priv;
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

    for (int i = 0; i < cnt; i++) {
        switch (type_list[i].type) {
        case TYPE_INT:
            json = cJSON_GetArrayItem(json, type_list[i].ival-1);
            break;
        case TYPE_CHARP:
            json = cJSON_GetObjectItem(json, type_list[i].cval);
            break;
        default:
            break;
        }
    }
    free(type_list);
    return json->valuestring;
}

static int js_get_int(struct config *c, ...)
{
    cJSON *json = (cJSON *)c->priv;
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

    for (int i = 0; i < cnt; i++) {
        switch (type_list[i].type) {
        case TYPE_INT:
            json = cJSON_GetArrayItem(json, type_list[i].ival-1);
            break;
        case TYPE_CHARP:
            json = cJSON_GetObjectItem(json, type_list[i].cval);
            break;
        default:
            break;
        }
    }
    free(type_list);
    return json->valueint;
}

static double js_get_double(struct config *c, ...)
{
    cJSON *json = (cJSON *)c->priv;
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

    for (int i = 0; i < cnt; i++) {
        switch (type_list[i].type) {
        case TYPE_INT:
            json = cJSON_GetArrayItem(json, type_list[i].ival-1);
            break;
        case TYPE_CHARP:
            json = cJSON_GetObjectItem(json, type_list[i].cval);
            break;
        default:
            break;
        }
    }
    free(type_list);
    return json->valuedouble;
}

static int js_get_boolean(struct config *c, ...)
{
    cJSON *json = (cJSON *)c->priv;
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

    for (int i = 0; i < cnt; i++) {
        switch (type_list[i].type) {
        case TYPE_INT:
            json = cJSON_GetArrayItem(json, type_list[i].ival-1);
            break;
        case TYPE_CHARP:
            json = cJSON_GetObjectItem(json, type_list[i].cval);
            break;
        default:
            break;
        }
    }
    free(type_list);

    if(!strcasecmp(json->valuestring, "true")) {
        return 1;
    } else {
        return 0;
    }
}

static void js_dump(struct config *c, FILE *f)
{
    cJSON *json = (cJSON *)c->priv;
    char *tmp = cJSON_Print(json);
    if (tmp) {
        printf("%s\n", tmp);
        free(tmp);
    }
}

static void js_unload(struct config *c)
{
    cJSON *json = (cJSON *)c->priv;
    if (json) {
        cJSON_Delete(json);
        json = NULL;
    }
}

struct config_ops json_ops = {
    .load        = js_load,
    .set_string  = js_set_string,
    .get_string  = js_get_string,
    .get_int     = js_get_int,
    .get_double  = js_get_double,
    .get_boolean = js_get_boolean,
    .del         = NULL,
    .dump        = js_dump,
    .save        = NULL,
    .unload      = js_unload,
};
