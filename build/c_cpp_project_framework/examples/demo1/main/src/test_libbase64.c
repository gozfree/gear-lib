/******************************************************************************
 * Copyright (C) 2014-2020 dianjixz <18637716021@163.com>
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
#include "libbase64.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "test.h"

void test_test_libbase64()
{
    char target[100] = {0};
    char target2[100] = {0};
    char source[]="hello world";
    int ret_bytes=0;
    printf("start base64_encode\n");
    memset(target, 0, sizeof(target));
    memset(target2, 0, sizeof(target2));
    ret_bytes = base64_encode(target, source, strlen(source));

    printf("src size: %zu , return byte: %d , target:%s\n", strlen(source), ret_bytes, target);

    ret_bytes = base64_decode(target2, target, ret_bytes);
    target[ret_bytes]='\0';
    printf("return byte: %d , target2: %s \n", ret_bytes, target2);
}


// int main(int argc, char **argv)
// {
//     test_test_libbase64();
//     return 0;
// }