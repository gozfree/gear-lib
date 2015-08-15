/*******************************************************************************
 * Copyright (C) 2014-2015
 * file:    test_libfoo.c
 * author:  gozfree <gozfree@163.com>
 * created: 2015-07-12 00:56
 * updated: 2015-07-12 00:56
 ******************************************************************************/
#include <stdio.h>

#include "libjpeg-ex.h"

void foo()
{
    struct yuv *yuv = yuv_new();
    struct jpeg *jpeg = jpeg_new(640, 480);
    jpeg_encode(jpeg, yuv);
    //use jpeg
    yuv_free(yuv);
    jpeg_free(jpeg);
}

int main(int argc, char **argv)
{
    foo();
    return 0;
}
