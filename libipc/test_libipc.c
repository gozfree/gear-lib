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
