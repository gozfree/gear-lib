/*****************************************************************************
 * Copyright (C) 2014-2015
 * file:    libconfig.h
 * author:  gozfree <gozfree@163.com>
 * created: 2015-09-29 23:49
 * updated: 2015-09-29 23:49
 *****************************************************************************/
#ifndef _LIBCONFIG_H_
#define _LIBCONFIG_H_

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
    int (*set_string)(struct config *c, const char *key, const char *val);
    char *(*get_string) (struct config *c, ...);
    int (*get_int)      (struct config *c, ...);
    double (*get_double)(struct config *c, ...);
    int (*get_boolean)  (struct config *c, ...);
    void (*dump)(struct config *c, FILE *f);
    void (*unload)(struct config *c);
} config_ops_t;

enum config_backend {
    CONFIG_LUA  = 0,
    CONFIG_JSON = 1,
    CONFIG_INI  = 2,
};

extern struct config *g_config;

struct config *conf_load(enum config_backend cb, const char *name);
int conf_set(struct config *c, const char *key, const char *val);
#define conf_get_int(c, ...) g_config->ops->get_int(c, __VA_ARGS__, NULL)
#define conf_get_string(c, ...) g_config->ops->get_string(c, __VA_ARGS__, NULL)
#define conf_get_double(c, ...) g_config->ops->get_double(c, __VA_ARGS__, NULL)
#define conf_get_boolean(c, ...) g_config->ops->get_boolean(c, __VA_ARGS__, NULL)
void conf_dump(struct config *c);
void conf_dump_to_file(FILE *f, struct config *c);
void conf_unload(struct config *c);




#ifdef __cplusplus
}
#endif
#endif
