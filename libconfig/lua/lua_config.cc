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
#include "libconfig.h"
#include "config_util.h"
#include "luatables.h"
#include <iostream>
#include <cstdlib>
#include <vector>
#include <sstream>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


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

    bool save() {
        std::string config_string = serialize();
        if (config_string.length() <= 0) {
            return false;
        }
        int fd = open(filename.c_str(), O_WRONLY|O_TRUNC, 0666);
        if (fd < 0) {
            return false;
        }
        size_t written = 0;
        size_t total = config_string.length();
        const char *str = config_string.c_str();
        while (written < total) {
            ssize_t retval = write(fd, (void*)(str + written), (total - written));
            if (retval > 0) {
                written += retval;
            } else if (retval <= 0) {
                printf("Failed to write file %s: %s", filename.c_str(), strerror(errno));
                break;
            }
        }
        return (written == total);
    }

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
        printf("LuaConfig %s failed!\n", name);
        return -1;
    }
    c->priv = (void *)lt;
    return 0;
}

static int lua_get_int(struct config *c, ...)
{
    LuaConfig *lt = (LuaConfig *)c->priv;
    lt = lt;
    LuaTableNode *node;
    LuaTable tbl;
    int ret;
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
    node = new LuaTableNode[cnt];
    if (cnt > 0) {
        if (type_list[0].type == TYPE_INT) {
            node[0] = (*lt)[type_list[0].ival];
        } else if (type_list[0].type == TYPE_CHARP) {
            node[0] = (*lt)[type_list[0].cval];
        }
    }

    for (int i = 1; i < cnt; i++) {
        switch (type_list[i].type) {
        case TYPE_INT:
            node[i] = node[i-1][type_list[i].ival];
            break;
        case TYPE_CHARP:
            node[i] = node[i-1][type_list[i].cval];
            break;
        default:
            break;
        }
    }
    free(type_list);
    ret = node[cnt-1].getDefault<int>(0);
    delete []node;
    return ret;
}

static char *lua_get_string(struct config *c, ...)
{
    LuaConfig *lt = (LuaConfig *)c->priv;
    lt = lt;
    LuaTableNode *node;
    LuaTable tbl;
    char *ret = NULL;
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
    node = new LuaTableNode[cnt];
    if (cnt > 0) {
        if (type_list[0].type == TYPE_INT) {
            node[0] = (*lt)[type_list[0].ival];
        } else if (type_list[0].type == TYPE_CHARP) {
            node[0] = (*lt)[type_list[0].cval];
        }
    }

    for (int i = 1; i < cnt; i++) {
        switch (type_list[i].type) {
        case TYPE_INT:
            node[i] = node[i-1][type_list[i].ival];
            break;
        case TYPE_CHARP:
            node[i] = node[i-1][type_list[i].cval];
            break;
        default:
            break;
        }
    }
    ret = (char *)node[cnt-1].getDefault<string>("").c_str();
    free(type_list);
    delete []node;
    return ret;
}


static double lua_get_double(struct config *c, ...)
{
    LuaConfig *lt = (LuaConfig *)c->priv;
    lt = lt;
    LuaTableNode *node;
    LuaTable tbl;
    double ret;
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
    node = new LuaTableNode[cnt];
    if (cnt > 0) {
        if (type_list[0].type == TYPE_INT) {
            node[0] = (*lt)[type_list[0].ival];
        } else if (type_list[0].type == TYPE_CHARP) {
            node[0] = (*lt)[type_list[0].cval];
        }
    }

    for (int i = 1; i < cnt; i++) {
        switch (type_list[i].type) {
        case TYPE_INT:
            node[i] = node[i-1][type_list[i].ival];
            break;
        case TYPE_CHARP:
            node[i] = node[i-1][type_list[i].cval];
            break;
        default:
            break;
        }
    }
    free(type_list);
    ret = node[cnt-1].getDefault<double>(0);
    delete []node;
    return ret;
}

static int lua_get_boolean(struct config *c, ...)
{
    LuaConfig *lt = (LuaConfig *)c->priv;
    lt = lt;
    LuaTableNode *node;
    LuaTable tbl;
    int ret;
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
    node = new LuaTableNode[cnt];
    if (cnt > 0) {
        if (type_list[0].type == TYPE_INT) {
            node[0] = (*lt)[type_list[0].ival];
        } else if (type_list[0].type == TYPE_CHARP) {
            node[0] = (*lt)[type_list[0].cval];
        }
    }

    for (int i = 1; i < cnt; i++) {
        switch (type_list[i].type) {
        case TYPE_INT:
            node[i] = node[i-1][type_list[i].ival];
            break;
        case TYPE_CHARP:
            node[i] = node[i-1][type_list[i].cval];
            break;
        default:
            break;
        }
    }
    free(type_list);
    ret = node[cnt-1].getDefault<bool>(false);
    delete []node;
    return ret;
}

static int lua_get_length(struct config *c, ...)
{
    LuaConfig *lt = (LuaConfig *)c->priv;
    lt = lt;
    LuaTableNode *node;
    LuaTable tbl;
    int ret;
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
    node = new LuaTableNode[cnt];
    if (cnt > 0) {
        if (type_list[0].type == TYPE_INT) {
            node[0] = (*lt)[type_list[0].ival];
        } else if (type_list[0].type == TYPE_CHARP) {
            node[0] = (*lt)[type_list[0].cval];
        }
    }

    for (int i = 1; i < cnt; i++) {
        switch (type_list[i].type) {
        case TYPE_INT:
            node[i] = node[i-1][type_list[i].ival];
            break;
        case TYPE_CHARP:
            node[i] = node[i-1][type_list[i].cval];
            break;
        default:
            break;
        }
    }
    free(type_list);
    ret = node[cnt-1].length();
    delete []node;
    return ret;
}
static int lua_save(struct config *c)
{
    LuaConfig *lt = (LuaConfig *)c->priv;
    return (lt->save()?0:-1);
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
    .get_length  = lua_get_length,
    .del         = NULL,
    .dump        = NULL,
    .save        = lua_save,
    .unload      = lua_unload,
};
