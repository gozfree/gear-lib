/******************************************************************************
 * Copyright (C) 2014-2015
 * file:    test_libcmd.c
 * author:  gozfree <gozfree@163.com>
 * created: 2015-10-31 16:59
 * updated: 2015-10-31 16:59
 ******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <liblog.h>
#include <libosal.h>
#include "libcmd.h"

static struct cmd *g_cmd = NULL;

static int do_ls(int argc, char **argv)
{
    char buf[100];
    system_noblock_with_result(argv, buf, sizeof(buf));
    printf("buf = %s\n", buf);
    return 0;
}

static int do_df(int argc, char **argv)
{
    char buf[1023];
    system_noblock_with_result(argv, buf, sizeof(buf));
    printf("buf = %s\n", buf);
    return 0;
}

static int do_foo(int argc, char **argv)
{
    return 0;
}

static int do_quit(int argc, char **argv)
{
    logi("quit\n");
    exit(0);
    return 0;
}

static void ctrl_c_op(int signo)
{
    if (g_cmd) {
        cmd_deinit(g_cmd);
    }
    exit(0);
}

int main(int argc, char **argv)
{
    signal(SIGINT, ctrl_c_op);
    g_cmd = cmd_init(100);
    if (!g_cmd) {
        loge("cmd_init failed!\n");
        return -1;
    }
    cmd_register(g_cmd, "ls", do_ls);
    cmd_register(g_cmd, "foo", do_foo);
    cmd_register(g_cmd, "df", do_df);
    cmd_register(g_cmd, "q", do_quit);
    int cnt = cmd_get_registered(g_cmd);
    logi("cmd_get_registered = %d\n", cnt);

    cmd_loop(g_cmd);

    return 0;
}
