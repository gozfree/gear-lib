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
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <libmacro.h>
#include <liblog.h>

#include "luatables.h"
#include "lua_config.h"


LuaConfig *LuaConfig::create(const char *config)
{
    LuaConfig *result = new LuaConfig();
    if (result && !result->init(config)) {
        delete result;
        result = NULL;
    }
    return result;
}

void LuaConfig::destroy()
{
    delete this;
}

bool LuaConfig::save()
{
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
            loge("Failed to write file %s: %s", filename.c_str(), strerror(errno));
            break;
        }
    }
    return (written == total);
}

bool LuaConfig::init(const char *config)
{
    *this = fromFile(config);
    return (luaRef != -1);
}

LuaConfig& LuaConfig::operator=(const LuaTable &table)
{
    *((LuaTable*)this) = table;
    return *this;
}
