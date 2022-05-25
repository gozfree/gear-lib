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
#ifndef LIBCONFIG_H
#define LIBCONFIG_H

#define LIBCONFIG_VERSION "0.1.0"

#include <libposix.h>
#include <stdio.h>
#include <stdbool.h>
#include <limits.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct config {
    struct config_ops *ops;
    char path[PATH_MAX];
    void *priv;
} config_t;

typedef struct config_ops {
    int  (*load)  (struct config *c, const char *name);
    void (*unload)(struct config *c);
    void (*dump)  (struct config *c, FILE *f);
    int  (*save)  (struct config *c);

    char*  (*get_string) (struct config *c, ...);
    int    (*set_string) (struct config *c, ...);
    int    (*get_int)    (struct config *c, ...);
    int    (*set_int)    (struct config *c, ...);
    double (*get_double) (struct config *c, ...);
    int    (*set_double) (struct config *c, ...);
    bool   (*get_boolean)(struct config *c, ...);
    int    (*set_boolean)(struct config *c, ...);

    void   (*del)     (struct config *c, const char *key);
} config_ops_t;


GEAR_API struct config *conf_load(const char *name);
GEAR_API void conf_unload(struct config *c);
GEAR_API int conf_set(struct config *c, const char *key, const char *val);
GEAR_API void conf_del(struct config *c, const char *key);
GEAR_API void conf_dump(struct config *c);
GEAR_API int conf_save(struct config *c);
GEAR_API void conf_dump_to_file(FILE *f, struct config *c);


/*
 * xxx = {
 *     yyy = {
 *         "aaa",
 *         "bbb",
 *     }
 * }
 * conf_get_type(c, "xxx", "yyy", 1) will get "aaa"
 * conf_get_type(c, "xxx", "yyy", 2) will get "bbb"
 * 0 or NULL will be recorgize end of args, must start array with 1
 */
extern struct config *g_config;
#define conf_get_int(c, ...) g_config->ops->get_int(c, __VA_ARGS__, NULL)
#define conf_set_int(c, ...) g_config->ops->set_int(c, __VA_ARGS__, NULL)
#define conf_get_string(c, ...) g_config->ops->get_string(c, __VA_ARGS__, NULL)
#define conf_set_string(c, ...) g_config->ops->set_string(c, __VA_ARGS__, NULL)
#define conf_get_double(c, ...) g_config->ops->get_double(c, __VA_ARGS__, NULL)
#define conf_set_double(c, ...) g_config->ops->set_double(c, __VA_ARGS__, NULL)
#define conf_get_boolean(c, ...) g_config->ops->get_boolean(c, __VA_ARGS__, NULL)
#define conf_set_boolean(c, ...) g_config->ops->set_boolean(c, __VA_ARGS__, NULL)
#define conf_get_length(c, ...) g_config->ops->get_length(c, __VA_ARGS__, NULL)


#ifdef __cplusplus
}
#endif
#endif
