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
#include "libmacro.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

void *mymalloc(size_t size)
{
    void *ptr = NULL;
    static int malloc_index = 0;
    malloc_index++;
    printf("malloc prev, malloc_index = %d\n", ++malloc_index);
    return ptr;
}

void foo()
{
    int i = 0;
    int len = 48;
    void *p = malloc(len);
    void *q = NULL;
    for (i = 0; i < len; i++) {
        *((char *)p +i) = i;
    }
    q = memdup(p, len);
    DUMP_BUFFER(p, len);
    DUMP_BUFFER(q, len);
}

void unused_macro_tests(void)
{
    int i;

    const char *duh = "Suck less!";
    char c = '@';

    int a, b;
    UNUSED(c, i);
    UNUSED(i);
    UNUSED(a, b);

    UNUSED(sizeof(char) == 1);
    UNUSED(sizeof(char) == 1, i);


    UNUSED(duh);
}

int main(int argc, char **argv)
{
    UNUSED(argc, argv);

    foo();
    printf("hello world\n");
    unused_macro_tests();
    printf("Little endian: %d\n", is_little_endian());
    return 0;
}

