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

int main(int argc, char **argv)
{
    kill(getpid(), SIGINT);
    ini_test();

    return 0;
}
