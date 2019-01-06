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
#include <stdio.h>
#include <string>

#include "libgconfig.h"

using namespace std;


Config::Config()
{

}

Config::~Config()
{

}

Config *Config::create(string path)
{
    string::size_type pos = path.rfind('.');
    if (pos == string::npos) {
        printf("%s is invalid.\n", path.c_str());
        return NULL;
    }
    string suffix = path.substr(pos+1);
    if (suffix == "lua") {
        return (Config *)LuaConfig::create(path.c_str());
    } else if (suffix == "json") {
        return (Config *)JsonConfig::create(path.c_str());
    } else {
        printf("type %s does not support!\n", suffix.c_str());
        return NULL;
    }
}

