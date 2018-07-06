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

static int ini_test(void)
{
    struct config *conf = conf_load("ini/example.ini");
    logi("ini_test\n");
    conf_dump(conf);
    conf_set_string(conf, "wine:aaaa", "ccc");
    conf_set_string(conf, "wine:aaaa", "ddd");
    conf_set_string(conf, "wine:aaaa", "eee");
    conf_del(conf, "wine:aaaa");
    logi("year = %d\n", conf_get_int(conf, "wine:year"));
    conf_save(conf);
    conf_unload(conf);

    return 0;
}

static int json_test(void)
{
    struct config *conf = conf_load("json/test.json");
    logi("json_test\n");
    logi("test1 = %s\n", conf_get_string(conf, "test1"));
    logi("test2 = %d\n", conf_get_int(conf, "test2"));
    logi("test3 = %f\n", conf_get_double(conf, "test3"));
    conf_unload(conf);

    return 0;
}

static int lua_test(void)
{
    struct config *conf = conf_load("lua/config.lua");
    logi("lua_test\n");

    logi("[type_3][sub_type_1][my]= %s\n", conf_get_string(conf, "type_3", "sub_type_1", "my"));
    logi("[type_2][sub_type_1][my]= %s\n", conf_get_string(conf, "type_2", "sub_type_1", "my"));


    logi("[type_1][type] = %s\n", conf_get_string(conf, "type_1", "type"));
    logi("[type_1][index] = %d\n", conf_get_int(conf, "type_1", "index"));

    logi("[type_2][type] = %s\n", conf_get_string(conf, "type_2", "type"));
    logi("[type_2][index] = %d\n", conf_get_int(conf, "type_2", "index"));
    logi("[type_2][my] = %s\n", conf_get_string(conf, "type_2", "my"));


    logi("md_source_type= %s\n", conf_get_string(conf, "md_source_type"));
    logi("md_enable = %d\n", conf_get_boolean(conf, "md_enable"));
    logi("fps= %f\n", conf_get_double(conf, "fps"));
    logi("yuv_path= %s\n", conf_get_string(conf, "yuv_path"));
    conf_save(conf);
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
