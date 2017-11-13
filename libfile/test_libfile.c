/******************************************************************************
 * Copyright (C) 2014-2015
 * file:    test_libfile.c
 * author:  gozfree <gozfree@163.com>
 * created: 2016-07-22 14:17:17
 * updated: 2016-07-22 14:17:17
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <errno.h>
#include "libfile.h"


static void foo(void)
{
    int i = 0;
    file_backend_type type;
    char buf[128] = {0};
    for (i = 0; i < 2; ++i) {
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
        if (iobuf) {
            printf("len=%zu, buf=%s\n", iobuf->iov_len, (char *)iobuf->iov_base);
        }
    }
}

static void foo2(void)
{
    struct file_systat *stat = file_get_systat("/run/nginx.pid");
    printf("total = %zuMB\n", stat->size_total/(1024*1024));
    printf("avail = %zuMB\n", stat->size_avail/(1024*1024));
    printf("free = %zuMB\n", stat->size_free/(1024*1024));
    printf("fs type name = %s\n", stat->fs_type_name);
}

static void foo3(void)
{
    printf("local path=%s\n", file_path_pwd());
    printf("suffix=%s\n", file_path_suffix(file_path_pwd()));
    printf("prefix=%s\n", file_path_prefix(file_path_pwd()));
}

int main(int argc, char **argv)
{
    uint64_t size = 1000;
    file_dir_remove("/home/hacker/github/snowball.repo/libraries/libfile/tmp");
    file_dir_size("/home/hacker/github/snowball.repo/libraries/libfile/", &size);
    printf("folder_size=%zu\n", size);
    file_dir_tree("/media/hacker/1B68-9956");
    foo();
    foo2();
    foo3();
    return 0;
}

