/*****************************************************************************
 * Copyright (C) 2014-2015
 * file:    libconfig.h
 * author:  gozfree <gozfree@163.com>
 * created: 2015-09-29 23:49
 * updated: 2015-09-29 23:49
 *****************************************************************************/
#ifndef _LIBCONFIG_H_
#define _LIBCONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct config {
    void *instance;

} config_t;

typedef struct config_ops {
    struct config *(*load)(const char *name);
    char *(*get_string)(struct config *c, const char *key);
    int (*get_int)(struct config *c, const char *key);
    double (*get_double)(struct config *c, const char *key);
    int (*get_boolean)(struct config *c, const char *key);
    void (*dump)(struct config *c);
    void (*unload)(struct config *c);
} config_ops_t;


struct config *conf_load(const char *name);
int conf_get_int(struct config *conf, const char *key);
char *conf_get_string(struct config *conf, const char *key);
double conf_get_double(struct config *conf, const char *key);
int conf_get_boolean(struct config *conf, const char *key);
void conf_dump(struct config *conf);
void conf_unload(struct config *conf);




#ifdef __cplusplus
}
#endif
#endif
