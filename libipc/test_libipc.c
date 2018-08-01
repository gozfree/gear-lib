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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
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
    struct ipc *ipc = ipc_create(IPC_CLIENT, IPC_SERVER_PORT);
    if (!ipc) {
        printf("ipc_create failed!\n");
        return -1;
    }
    while (loop) {
        memset(buf, 0, sizeof(buf));
        printf("hack shell$ ");
        scanf("%s", cmd);
        if (!strcmp(cmd, "quit")) {
            break;
        }
        ipc_call(ipc, IPC_SHELL_HELP, cmd, sizeof(cmd), buf, sizeof(buf));
        printf("%s\n", buf);
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
    printf("calc string=\"%d %c %d\"\n", calc.left, calc.opcode, calc.right);
    struct ipc *ipc = ipc_create(IPC_CLIENT, IPC_SERVER_PORT);
    printf("before ipc_call\n");
    if (0 != ipc_call(ipc, IPC_CALC, &calc, sizeof(calc), &ret, sizeof(ret))) {
        printf("ipc_call %d failed!\n", IPC_CALC);
        return -1;
    }
    printf("ipc_call IPC_CALC success!\n");
    printf("return value = %d\n", ret);
    ipc_destroy(ipc);
    return 0;
}

int main(int argc, char **argv)
{
    //foo();
    shell_test();

    return 0;
}
