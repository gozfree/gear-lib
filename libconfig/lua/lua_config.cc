/******************************************************************************
 * Copyright (C) 2014-2016
 * file:    lua_config.c
 * author:  gozfree <gozfree@163.com>
 * created: 2016-05-07 00:05
 * updated: 2016-05-07 00:05
 ******************************************************************************/
#include <iostream>
#include <cstdlib>
#include <vector>
#include <sstream>
#include <stdarg.h>
#include <string.h>
#include <libmacro.h>
#include <liblog.h>

#include "../libconfig.h"
#include "luatables.h"

using namespace std;

class LuaConfig: public LuaTable
{
  public:
    static LuaConfig* create(const char *config) {
        LuaConfig *result = new LuaConfig();
        if (result && !result->init(config)) {
            delete result;
            result = NULL;
        }
        return result;
    };
    void destroy() {
        delete this;
    };

  public:
    virtual ~LuaConfig(){}

  private:
    LuaConfig(){};
    bool init(const char *config) {
        *this = fromFile(config);
        return (luaRef != -1);
    };
    LuaConfig& operator=(const LuaTable &table) {
        *((LuaTable*)this) = table;
        return *this;
    };
};


static int lua_load(struct config *c, const char *name)
{
    LuaConfig *lt = LuaConfig::create(name);
    if (!lt) {
        loge("LuaConfig %s failed!\n", name);
        return -1;
    }
    c->priv = (void *)lt;
    return 0;
}

static int lua_get_int(struct config *c, ...)
{
    LuaConfig *lt = (LuaConfig *)c->priv;
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
        ret = (*lt)[key[0]].getDefault<int>(0);
        break;
    case 2:
        ret = (*lt)[key[0]][key[1]].getDefault<int>(0);
        break;
    case 3:
        ret = (*lt)[key[0]][key[1]][key[2]].getDefault<int>(0);
        break;
    case 4:
        ret = (*lt)[key[0]][key[1]][key[2]][key[3]].getDefault<int>(0);
        break;
    default:
        break;
    }
    free(key);
    return ret;
}

static char *lua_get_string(struct config *c, ...)
{
    LuaConfig *lt = (LuaConfig *)c->priv;
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
        ret = (char *)(*lt)[key[0]].getDefault<string>("").c_str();
        break;
    case 2:
        ret = (char *)(*lt)[key[0]][key[1]].getDefault<string>("").c_str();
        break;
    case 3:
        ret = (char *)(*lt)[key[0]][key[1]][key[2]].getDefault<string>("").c_str();
        break;
    case 4:
        ret = (char *)(*lt)[key[0]][key[1]][key[2]][key[3]].getDefault<string>("").c_str();
        break;
    case 0:
    default:
        break;
    }
    free(key);
    return strdup(ret);
}

static double lua_get_double(struct config *c, ...)
{
    LuaConfig *lt = (LuaConfig *)c->priv;
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
        ret = (*lt)[key[0]].getDefault<double>(0);
        break;
    case 2:
        ret = (*lt)[key[0]][key[1]].getDefault<double>(0);
        break;
    case 3:
        ret = (*lt)[key[0]][key[1]][key[2]].getDefault<double>(0);
        break;
    case 4:
        ret = (*lt)[key[0]][key[1]][key[2]][key[3]].getDefault<double>(0);
        break;
    case 0:
    default:
        break;
    }
    free(key);
    return ret;
}

static int lua_get_boolean(struct config *c, ...)
{
    LuaConfig *lt = (LuaConfig *)c->priv;
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
        ret = (*lt)[key[0]].getDefault<bool>(false);
        break;
    case 2:
        ret = (*lt)[key[0]][key[1]].getDefault<bool>(false);
        break;
    case 3:
        ret = (*lt)[key[0]][key[1]][key[2]].getDefault<bool>(false);
        break;
    case 4:
        ret = (*lt)[key[0]][key[1]][key[2]][key[3]].getDefault<bool>(false);
        break;
    case 0:
    default:
        break;
    }
    free(key);
    return ret;
}

static void lua_unload(struct config *c)
{
    LuaConfig *lt = (LuaConfig *)c->priv;
    lt->destroy();
}

struct config_ops lua_ops = {
    .load        = lua_load,
    .set_string  = NULL,
    .get_string  = lua_get_string,
    .get_int     = lua_get_int,
    .get_double  = lua_get_double,
    .get_boolean = lua_get_boolean,
    .dump        = NULL,
    .unload      = lua_unload,
};
