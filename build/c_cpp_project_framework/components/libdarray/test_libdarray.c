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
#include "libdarray.h"
#include "libserializer.h"
#include <stdio.h>
#include <stdlib.h>

static int foo()
{
    struct serializer *s, ss;
    s = &ss;
    serializer_array_init(s);
    s_w8(s, 1);           /* Version */
    s_write(s, "FLV", 3);
    serializer_array_deinit(s);

    return 0;
}


int main(int argc, char **argv)
{
    int j;
    DARRAY(int) i;
    foo();
    da_init(i);
    for (j = 0; j < 10; j++) {
        da_push_back(i, &j);
    }
    da_erase(i, 0);
    da_erase(i, 4);
    for (j = 10; j < 20; j++) {
        da_push_back_array(i, &j, 1);
    }
    for (j = 0; j < 20; j++) {
        printf("tmp=%x\n", i.array[j]);
    }
    da_free(i);
    return 0;
}
