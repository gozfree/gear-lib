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

#include "libfile.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <errno.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>


static void foo(void)
{
    int len = 0;
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
        struct file *fw = file_open("/tmp/lsusb", F_CREATE);
        file_write(fw, "hello file\n", 11);
        file_sync(fw);
        file_seek(fw, 0, SEEK_SET);
        memset(buf, 0, sizeof(buf));
        len = file_read(fw, buf, sizeof(buf));
        printf("read len = %d, buf = %s", len, buf);
        file_close(fw);

        struct file *f = file_open("/tmp/lsusb", F_RDONLY);
        memset(buf, 0, sizeof(buf));
        len = file_read(f, buf, sizeof(buf));
        printf("read len = %d, buf = %s", len, buf);
        printf("len=%zu\n", file_get_size("/tmp/lsusb"));
        struct iovec *iobuf = file_dump("/tmp/lsusb");
        if (iobuf) {
            //printf("len=%zu, buf=%s\n", iobuf->iov_len, (char *)iobuf->iov_base);
        }
    }
}

static void foo2(void)
{
    struct file_systat *stat = file_get_systat("./Makefile");
    printf("total = %" PRIu64 "MB\n", stat->size_total/(1024*1024));
    printf("avail = %" PRIu64 "MB\n", stat->size_avail/(1024*1024));
    printf("free = %" PRIu64 "MB\n", stat->size_free/(1024*1024));
    printf("fs type name = %s\n", stat->fs_type_name);
}

static void foo3(void)
{
    printf("local path=%s\n", file_path_pwd());
    printf("suffix=%s\n", file_path_suffix(file_path_pwd()));
    printf("prefix=%s\n", file_path_prefix(file_path_pwd()));
}

static void foo4(void)
{
    int len = 0;
    char buf[128] = {0};
    struct file *fw = file_open("/tmp/lsusb", F_APPEND);
    file_write(fw, "hello file\n", 11);
    file_sync(fw);
    file_seek(fw, 0, SEEK_SET);
    memset(buf, 0, sizeof(buf));
    len = file_read(fw, buf, sizeof(buf));
    printf("read len = %d, buf = %s", len, buf);
    file_close(fw);
}

static void foo5(void)
{
    struct file_info info;
    file_get_info("/tmp/lsusb", &info);
    printf("info->size = %" PRIu64 "\n", info.size);
    printf("info->type = %d\n", info.type);
    printf("info->time_modify = %" PRIu64 "\n", info.modify_sec);
    printf("info->time_access = %" PRIu64 "\n", info.access_sec);
}

int main(int argc, char **argv)
{
    uint64_t size = 1000;
    file_dir_size("./", &size);
    printf("folder_size=%" PRIu64 "\n", size);
    file_dir_tree("./");

    foo5();
    foo4();
    foo();
    foo2();
    foo3();
    if (0 != file_create("jjj.c")) {
        printf("file_create failed!\n");
    }
    return 0;
}

