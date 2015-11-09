/******************************************************************************
 * Copyright (C) 2014-2015
 * file:    test_libdlmod.c
 * author:  gozfree <gozfree@163.com>
 * created: 2015-11-09 18:52
 * updated: 2015-11-09 18:52
 ******************************************************************************/
#include "libdlmod.h"

void test()
{
    void *handle = dl_load("/usr/local/lib/libthread.so");
    dl_unload(handle);

}

int main(int argc, char **argv)
{
    test();
    return 0;
}
