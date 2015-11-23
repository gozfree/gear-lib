/******************************************************************************
 * Copyright (C) 2014-2015
 * file:    test_libipc.c
 * author:  gozfree <gozfree@163.com>
 * created: 2015-11-10 16:29:41
 * updated: 2015-11-10 16:29:41
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include "libipc.h"
#include "libipc_stub.h"

int foo()
{
    struct ipc *ipc = ipc_create(IPC_SENDER);
    //ipc_call(ipc, IPC_TEST, NULL, 0, NULL, 0);
    ipc_call(ipc, IPC_GET_CONNECT_LIST, NULL, 0, NULL, 0);
    return 0;
}

int main(int argc, char **argv)
{
    foo();
    return 0;
}
