/******************************************************************************
 * Copyright (C) 2014-2020 Zhifeng Gong <gozfree@163.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 ******************************************************************************/
#include "libconfig.h"
#include "config_util.h"
#include "cJSON.h"
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

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
    cJSON *json;
    char *buf = read_file(name);
    if (!buf) {
        printf("read_file %s failed!\n", name);
        return -1;
    }
    json = cJSON_Parse(buf);
    if (!json) {
        printf("cJSON_Parse failed!\n");
        free(buf);
        return -1;
    }
    free(buf);
    c->priv = (void *)json;
    strncpy(c->path, name, sizeof(c->path));
    return 0;
}

static int js_set_string(struct config *c, ...)
{
    cJSON *json = (cJSON *)c->priv;
    struct int_charp *type_list = NULL;
    struct int_charp mix;
    int cnt = 0;
    int i;
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

    for (i = 0; i < cnt-1; i++) {
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
    int i;
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

    for (i = 0; i < cnt; i++) {
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
    int i;
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

    for (i = 0; i < cnt; i++) {
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
    int i;
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

    for (i = 0; i < cnt; i++) {
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
    int i;
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

    for (i = 0; i < cnt; i++) {
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
    js_load,
    js_set_string,
    js_get_string,
    js_get_int,
    js_get_double,
    js_get_boolean,
    NULL,
    NULL,
    js_dump,
    NULL,
    js_unload,
};
