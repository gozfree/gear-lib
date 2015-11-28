/******************************************************************************
 * Copyright (C) 2014-2015
 * file:    test_libipc.c
 * author:  gozfree <gozfree@163.com>
 * created: 2015-11-10 16:29:41
 * updated: 2015-11-10 16:29:41
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <liblog.h>
#include "libipc.h"
#include "libipc_stub.h"


struct calc_args {
    int32_t left;
    int32_t right;
    int32_t opcode;
};

int shell_test()
{
    char buf[1024];
    char cmd[512];
    int loop = 1;
    struct ipc *ipc = ipc_create(IPC_CLIENT, 5555);
    if (!ipc) {
        loge("ipc_create failed!\n");
        return -1;
    }
    while (loop) {
        memset(buf, 0, sizeof(buf));
        printf("hack shell$ ");
        scanf("%s", cmd);
        printf("cmd = %s\n", cmd);
        ipc_call(ipc, IPC_SHELL_HELP, cmd, sizeof(cmd), buf, sizeof(buf));
        printf("ret = %s\n", buf);
    }
    ipc_destroy(ipc);
    return 0;

}

int foo()
{
    struct calc_args calc;
    calc.left = 123;
    calc.right = 321;
    calc.opcode = '+';
    int ret;
    logi("calc string=\"%d %c %d\"\n", calc.left, calc.opcode, calc.right);
    struct ipc *ipc = ipc_create(IPC_CLIENT, 5555);
    logi("before ipc_call\n");
    if (0 != ipc_call(ipc, IPC_CALC, &calc, sizeof(calc), &ret, sizeof(ret))) {
        loge("ipc_call %d failed!\n", IPC_CALC);
        return -1;
    }
    logi("ipc_call IPC_CALC success!\n");
    logi("return value = %d\n", ret);
    ipc_destroy(ipc);
    return 0;
}

int main(int argc, char **argv)
{
    //foo();
    shell_test();

    return 0;
}
