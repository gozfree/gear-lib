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
#include <string.h>
#include "libringbuffer.h"

int foo()
{
    struct ringbuffer *rb = rb_create(1024);
    const char *tmp = "hello world";
    ssize_t ret = 0;
    size_t len = 0;
    for (int i = 0; i < 100; i++) {
        printf("free=%zu, used=%zu\n", rb_get_space_free(rb), rb_get_space_used(rb));
        ret = rb_write(rb, tmp, strlen(tmp));
        if (ret < 0) {
            break;
        }
    }
    printf("dump = %s\n", (char *)rb_dump(rb, &len));
    printf("rb_write len=%zu\n", len);
    char tmp2[9];
    memset(tmp2, 0, sizeof(tmp2));
    rb_read(rb, tmp2, sizeof(tmp2)-1);
    printf("rb_read str=%s\n", tmp2);
    memset(tmp2, 0, sizeof(tmp2));
    rb_read(rb, tmp2, sizeof(tmp2)-1);
    printf("rb_read str=%s\n", tmp2);
    return 0;
}

int main(int argc, char **argv)
{
    foo();
    return 0;
}
