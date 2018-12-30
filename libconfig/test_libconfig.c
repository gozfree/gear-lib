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
#include "libconfig.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/types.h>
#include <unistd.h>

static int ini_test(void)
{
    struct config *conf = conf_load("ini/example.ini");
    printf("ini_test\n");
    conf_dump(conf);
    conf_set_string(conf, "wine:aaaa", "ccc");
    conf_set_string(conf, "wine:aaaa", "ddd");
    conf_set_string(conf, "wine:aaaa", "eee");
    conf_del(conf, "wine:aaaa");
    printf("year = %d\n", conf_get_int(conf, "wine:year"));
    conf_save(conf);
    conf_unload(conf);

    return 0;
}

static int json_test(void)
{
    struct config *conf = conf_load("json/all.json");
    printf("json_test\n");
    printf("id = %s\n", conf_get_string(conf, "test", "rgn", 1, "id"));
    printf("port= %d\n", conf_get_int(conf, "test", "rgn", 1, "port"));
    conf_set_string(conf, "test", "rgn", 1, "id", "update");
    conf_unload(conf);

    return 0;
}

static int lua_test(void)
{
#ifdef ENABLE_LUA
    struct config *conf = conf_load("lua/config.lua");
    printf("lua_test\n");

    printf("[type_3][sub_type_1][my]= %s\n", conf_get_string(conf, "type_3", "sub_type_1", "my"));
    printf("[type_2][sub_type_1][my]= %s\n", conf_get_string(conf, "type_2", "sub_type_1", "my"));


    printf("[type_1][type] = %s\n", conf_get_string(conf, "type_1", "type"));
    printf("[type_1][index] = %d\n", conf_get_int(conf, "type_1", "index"));

    printf("[type_2][type] = %s\n", conf_get_string(conf, "type_2", "type"));
    printf("[type_2][index] = %d\n", conf_get_int(conf, "type_2", "index"));
    printf("[type_2][my] = %s\n", conf_get_string(conf, "type_2", "my"));


    printf("md_source_type= %s\n", conf_get_string(conf, "md_source_type"));
    printf("md_enable = %d\n", conf_get_boolean(conf, "md_enable"));
    printf("fps= %f\n", conf_get_double(conf, "fps"));
    printf("yuv_path= %s\n", conf_get_string(conf, "yuv_path"));
    conf_save(conf);
    conf_unload(conf);
#endif

    return 0;
}

int main(int argc, char **argv)
{
    ini_test();
    json_test();
    lua_test();

    return 0;
}
