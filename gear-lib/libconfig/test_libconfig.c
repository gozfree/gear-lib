/******************************************************************************
 * Copyright (C) 2014-2020 Zhifeng Gong <gozfree@163.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 ******************************************************************************/
#include "libconfig.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>

static int ini_test(void)
{
    struct config *conf = conf_load("ini/example.ini");
    printf("ini_test\n");
    conf_set_string(conf, "wine:year", "1122");
    conf_set_string(conf, "wine:aaaa", "ddd");
    conf_set_string(conf, "wine:eeee", "1.234");
    conf_dump(conf);
    conf_del(conf, "wine:aaaa");
    printf("year = %d\n", conf_get_int(conf, "wine:year"));
    printf("grape = %s\n", conf_get_string(conf, "wine:grape"));
    printf("alcohol = %f\n", conf_get_double(conf, "wine:alcohol"));
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

    printf("yuv_path= %s\n", conf_get_string(conf, "yuv_path"));

    printf("md_source_type= %s\n", conf_get_string(conf, "md_source_type"));
    printf("md_enable = %d\n", conf_get_boolean(conf, "md_enable"));
    printf("fps= %f\n", conf_get_double(conf, "fps"));
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
