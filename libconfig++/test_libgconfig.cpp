/******************************************************************************
 * Copyright (C) 2014-2015
 * file:    test_libconfig++.c
 * author:  gozfree <gozfree@163.com>
 * created: 2017-03-30 15:00:24
 * updated: 2017-03-30 15:00:24
 *****************************************************************************/
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <liblog.h>
#include "libgconfig.h"
using namespace std;

static void lua_test()
{
    LuaConfig *conf = Config::create("lua/config.lua");
    logi("yuv_path= %s\n", (*conf)["yuv_path"].getDefault<string>("").c_str());
    logi("[type_3][sub_type_1][my] = %s\n", (*conf)["type_3"]["sub_type_1"]["my"].getDefault<string>("").c_str());

    //lc.save();
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
    ofstream ofs;
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
