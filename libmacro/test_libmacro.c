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
#include "libmacro.h"
#include <stdio.h>
#include <stdlib.h>

void *mymalloc(size_t size)
{
    static int malloc_index = 0;
    malloc_index++;
    printf("malloc prev, malloc_index = %d\n", ++malloc_index);
    void *ptr = CALL(malloc, size);
    printf("malloc post\n");
    return ptr;
}

void foo()
{
    void *p = mymalloc(10);
    UNUSED(p);
}

void foo2()
{
    char *cmd = "date";
    char buf[64];
    system_with_result(cmd, buf, sizeof(buf));
    printf("buf = %s\n", buf);
}

int main(int argc, char **argv)
{
    foo();
    foo2();
    return 0;
}
