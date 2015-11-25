/******************************************************************************
 * Copyright (C) 2014-2015
 * file:    test_libipc.c
 * author:  gozfree <gozfree@163.com>
 * created: 2015-11-10 16:29:41
 * updated: 2015-11-10 16:29:41
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
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
    struct calc_args calc;
    calc.left = 1;
    calc.right = 2;
    calc.opcode = '+';
    int ret;
    logi("calc left = %d, right = %d, opcode = %c\n", calc.left, calc.right, calc.opcode);
    
    struct ipc *ipc = ipc_create(IPC_SENDER);
    //ipc_call(ipc, IPC_TEST, NULL, 0, NULL, 0);
    ipc_call(ipc, IPC_CALC, &calc, sizeof(calc), &ret, sizeof(ret));
    return 0;
}

int main(int argc, char **argv)
{
    foo();
    return 0;
}
