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
#include "libconfig.h"

int ini_test()
{
    struct config *conf = conf_load("ini/example.ini");
    conf_dump(conf);
    printf("year = %d\n", conf_get_int(conf, "wine:year"));

    conf_unload(conf);

    return 0;
}

int json_test()
{
    struct config *conf = conf_load("json/test.json");
    printf("test1 = %s\n", conf_get_string(conf, "test1"));
    printf("test2 = %d\n", conf_get_int(conf, "test2"));
    printf("test3 = %f\n", conf_get_double(conf, "test3"));
    conf_unload(conf);

    return 0;
}

int lua_test()
{
    struct config *conf = conf_load("lua/config.lua");
    printf("md_enable = %d\n", conf_get_boolean(conf, "md_enable"));
    printf("md_source_type= %s\n", conf_get_string(conf, "md_source_type"));
    printf("fps= %f\n", conf_get_double(conf, "fps"));
    conf_unload(conf);

    return 0;
}

int main(int argc, char **argv)
{
//    ini_test();
//    json_test();
    lua_test();

    return 0;
}
