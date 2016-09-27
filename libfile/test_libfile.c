/******************************************************************************
 * Copyright (C) 2014-2015
 * file:    test_libfile.c
 * author:  gozfree <gozfree@163.com>
 * created: 2016-07-22 14:17:17
 * updated: 2016-07-22 14:17:17
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include "libfile.h"


static void foo(void)
{
    int i = 0;
    file_backend_type type;
    char buf[128] = {0};
    for(i = 0; i < 2; ++i)
    {
        if (i == 0)
            type = FILE_BACKEND_IO;
        else if (i == 1)
            type = FILE_BACKEND_FIO;
        file_backend(type);
        printf("backend=%d\n", type);
        struct file *fw = file_open("/tmp/lsusb", F_RDWR);
        file_write(fw, "hello file\n", 11);
        file_sync(fw);
        file_seek(fw, 0, SEEK_SET);
        file_read(fw, buf, sizeof(buf));
        printf("buf =%s", buf);
        file_close(fw);

        struct file *f = file_open("/tmp/lsusb", F_RDONLY);
        file_read(f, buf, sizeof(buf));
        printf("buf =%s", buf);
        printf("len=%zu\n", file_get_size("/tmp/lsusb"));
        struct iovec *iobuf = file_dump("/tmp/lsusb");
        printf("len=%zu, buf=%s\n", iobuf->iov_len, (char *)iobuf->iov_base);
    }
}

int main(int argc, char **argv)
{
    foo();
    return 0;
}

