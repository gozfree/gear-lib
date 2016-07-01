/******************************************************************************
 * Copyright (C) 2014-2015
 * file:    test_libringbuffer.c
 * author:  gozfree <gozfree@163.com>
 * created: 2016-06-29 11:42:50
 * updated: 2016-06-29 11:42:50
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libringbuffer.h"

int foo()
{
    struct ringbuffer *rb = rb_create(1024);
    const char *tmp = "hello world";
    ssize_t ret = rb_write(rb, tmp, strlen(tmp));
    printf("rb_write len=%zu\n", ret);
    ret = rb_write(rb, tmp, strlen(tmp));
    printf("rb_write len=%zu\n", ret);
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
