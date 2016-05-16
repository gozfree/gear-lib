/*****************************************************************************
 * Copyright (C) 2014-2015
 * file:    test_libconfig.c
 * author:  gozfree <gozfree@163.com>
 * created: 2015-05-31 19:42
 * updated: 2015-05-31 19:42
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/types.h>
#include <unistd.h>
#include <liblog.h>
#include "libconfig.h"

int ini_test()
{
    struct config *conf = conf_load(CONFIG_INI, "ini/example.ini");
    logi("ini_test\n");
    conf_dump(conf);
    logi("year = %d\n", conf_get_int(conf, "wine:year"));

    conf_unload(conf);

    return 0;
}

int json_test()
{
    struct config *conf = conf_load(CONFIG_JSON, "json/test.json");
    logi("json_test\n");
    logi("test1 = %s\n", conf_get_string(conf, "test1"));
    logi("test2 = %d\n", conf_get_int(conf, "test2"));
    logi("test3 = %f\n", conf_get_double(conf, "test3"));
    conf_unload(conf);

    return 0;
}

int lua_test()
{
    struct config *conf = conf_load(CONFIG_LUA, "lua/config.lua");
    logi("lua_test\n");
    logi("type = %d\n", conf_get_int(conf, "type_2", "index"));
    logi("md_enable = %d\n", conf_get_boolean(conf, "md_enable"));
    logi("md_source_type= %s\n", conf_get_string(conf, "md_source_type"));
    logi("fps= %f\n", conf_get_double(conf, "fps"));
    conf_unload(conf);

    return 0;
}

int main(int argc, char **argv)
{
    ini_test();
    json_test();
    lua_test();

    return 0;
}
