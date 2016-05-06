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
#include <libgzf.h>

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


static struct config *load(const char *name)
{
    LuaConfig *lt = LuaConfig::create(name);
    struct config *c = CALLOC(1, struct config);
    c->instance = (void *)lt;
    return c;
}

static int set_string(struct config *c, const char *key, const char *val)
{
    return 0;
}

static char *get_string(struct config *c, const char *key)
{
    LuaConfig *lt = (LuaConfig *)c->instance;
    return (char *)(*lt)[key].getDefault<string>("").c_str();
}

static int get_int(struct config *c, const char *key)
{
    LuaConfig *lt = (LuaConfig *)c->instance;
    return (*lt)[key].getDefault<int>(0);
}

static double get_double(struct config *c, const char *key)
{
    LuaConfig *lt = (LuaConfig *)c->instance;
    return (*lt)[key].getDefault<double>(0);
}

static int get_boolean(struct config *c, const char *key)
{
    LuaConfig *lt = (LuaConfig *)c->instance;
    return (*lt)[key].getDefault<bool>(false);
}

static void unload(struct config *c)
{
    if (c) {
        free(c);
    }
}

static void dump(struct config *c, FILE *f)
{

}


struct config_ops lua_ops = {
    .load = load,
    .set_string = set_string,
    .get_string = get_string,
    .get_int = get_int,
    .get_double = get_double,
    .get_boolean = get_boolean,
    .dump = dump,
    .unload = unload,
};
