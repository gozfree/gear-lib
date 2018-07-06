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
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <liblog.h>
#include "libgconfig.h"
using std::string;

static void lua_test()
{
    LuaConfig *conf = LuaConfig::create("lua/config.lua");
    logi("url = %s\n", (*conf)["url"].get<string>("").c_str());
    (*conf)["auto_live"] = true;
    (*conf)["url"] = string("rtsp://xxx");
    (*conf)["crypto"] = string("md5");
    (*conf)["desc"] = string("stream");
    conf->save();
    conf->destroy();
}

static void json_test()
{
    JsonConfig *conf = Config::create("json/config.json");
    logi("[name] = %s\n", (*conf)["name"].asString().c_str());

    Json::Value root;
    Json::Value name;
    name["name"] = "hello";
    name["age"] = 10;
    root.append(name);


    Json::FastWriter writer;
    std::string json_file = writer.write(root);
    std::ofstream ofs;
    ofs.open("test1.json");
    assert(ofs.is_open());
    ofs<<json_file;

    conf->destroy();
}

int main(int argc, char **argv)
{
    lua_test();
    json_test();
    return 0;
}
