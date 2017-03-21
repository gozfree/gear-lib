/*****************************************************************************
 * Copyright (C) 2014-2015
 * file:    libconfig.h
 * author:  gozfree <gozfree@163.com>
 * created: 2015-09-29 23:49
 * updated: 2015-09-29 23:49
 *****************************************************************************/
#ifndef LIBCONFIG_H
#define LIBCONFIG_H

#include <stdio.h>


#ifdef __cplusplus
extern "C" {
#endif

typedef struct config {
    struct config_ops *ops;
    void *priv;
} config_t;

typedef struct config_ops {
    int (*load)(struct config *c, const char *name);
    int (*set_string)(struct config *c, const char *key, const char *val, const char *end);
    char *(*get_string) (struct config *c, ...);
    int (*get_int)      (struct config *c, ...);
    double (*get_double)(struct config *c, ...);
    int (*get_boolean)  (struct config *c, ...);
    void (*del)(struct config *c, const char *key);
    void (*dump)(struct config *c, FILE *f);
    int (*save)(struct config *c);
    void (*unload)(struct config *c);
} config_ops_t;

extern struct config *g_config;

struct config *conf_load(const char *name);
int conf_set(struct config *c, const char *key, const char *val);
#define conf_get_int(c, ...) g_config->ops->get_int(c, __VA_ARGS__, NULL)
#define conf_get_string(c, ...) g_config->ops->get_string(c, __VA_ARGS__, NULL)
#define conf_set_string(c, ...) g_config->ops->set_string(c, __VA_ARGS__, NULL)
#define conf_get_double(c, ...) g_config->ops->get_double(c, __VA_ARGS__, NULL)
#define conf_get_boolean(c, ...) g_config->ops->get_boolean(c, __VA_ARGS__, NULL)
void conf_del(struct config *c, const char *key);
void conf_dump(struct config *c);
int conf_save(struct config *c);
void conf_dump_to_file(FILE *f, struct config *c);
void conf_unload(struct config *c);




#ifdef __cplusplus
}
#endif
#endif
