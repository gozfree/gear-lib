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

int foo()
{
    int i = 0;
    struct calc_args calc;
    calc.left = 123;
    calc.right = 321;
    calc.opcode = '+';
    int ret;
    logi("calc string=\"%d %c %d\"\n", calc.left, calc.opcode, calc.right);
    struct ipc *ipc = ipc_create(IPC_CLIENT, 5555);
    logi("before ipc_call\n");
    for (i = 0; i < 3; ++i) {
        if (0 != ipc_call(ipc, IPC_CALC, &calc, sizeof(calc), &ret, sizeof(ret))) {
            loge("ipc_call %d failed!\n", IPC_CALC);
            return -1;
        }
    }
    logi("ipc_call IPC_CALC success!\n");
    logi("return value = %d\n", ret);
    ipc_destroy(ipc);
    return 0;
}

int main(int argc, char **argv)
{
    foo();

    return 0;
}
