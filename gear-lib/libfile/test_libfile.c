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
#include "libfile.h"
#ifdef ENABLE_FILEWATCHER
#include "libfilewatcher.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#if defined (__linux__) || defined (__CYGWIN__)
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#endif


static void foo(void)
{
    int len = 0;
    int i = 0;
    file_backend_type type;
    struct file *fw;
    struct file *f;
    struct iovec *iobuf;
    char buf[128] = {0};
    for (i = 0; i < 2; ++i) {
        if (i == 0)
            type = FILE_BACKEND_IO;
        else if (i == 1)
            type = FILE_BACKEND_FIO;
        file_backend(type);
        printf("backend=%d\n", type);
        fw = file_open("lsusb", F_CREATE);
        file_write(fw, "hello file\n", 11);
        file_sync(fw);
        file_seek(fw, 0, SEEK_SET);
        memset(buf, 0, sizeof(buf));
        len = file_read(fw, buf, sizeof(buf));
        printf("read len = %d, buf = %s", len, buf);
        file_close(fw);

        f = file_open("lsusb", F_RDONLY);
        memset(buf, 0, sizeof(buf));
        len = file_read(f, buf, sizeof(buf));
        printf("read len = %d, buf = %s", len, buf);
        printf("len=%zu\n", file_get_size("lsusb"));

        iobuf = file_dump("lsusb");
        if (iobuf) {
            //printf("len=%zu, buf=%s\n", iobuf->iov_len, (char *)iobuf->iov_base);
        }
    }
}

static void foo2(void)
{
    struct file_systat *stat = file_get_systat("./Makefile");
    if (!stat)
        return;
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
    struct file *fw = file_open("lsusb", F_APPEND);
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
    file_get_info("lsusb", &info);
    printf("info->size = %" PRIu64 "\n", info.size);
    printf("info->type = %d\n", info.type);
    printf("info->time_modify = %" PRIu64 "\n", info.modify_sec);
    printf("info->time_access = %" PRIu64 "\n", info.access_sec);
}

#ifdef ENABLE_FILEWATCHER
#define ROOT_DIR		"./"
static struct fw *_fw = NULL;


void on_change(struct fw *fw, enum fw_type type, char *path)
{
    switch (type) {
    case FW_CREATE_DIR:
        printf("[CREATE DIR] %s\n", path);
        break;
    case FW_CREATE_FILE:
        printf("[CREATE FILE] %s\n", path);
        break;
    case FW_DELETE_DIR:
        printf("[DELETE DIR] %s\n", path);
        break;
    case FW_DELETE_FILE:
        printf("[DELETE FILE] %s\n", path);
        break;
    case FW_MOVE_FROM_DIR:
        printf("[MOVE FROM DIR] %s\n", path);
        break;
    case FW_MOVE_TO_DIR:
        printf("[MOVE TO DIR] %s\n", path);
        break;
    case FW_MOVE_FROM_FILE:
        printf("[MOVE FROM FILE] %s\n", path);
        break;
    case FW_MOVE_TO_FILE:
        printf("[MOVE TO FILE] %s\n", path);
        break;
    case FW_MODIFY_FILE:
        printf("[MODIFY FILE] %s\n", path);
        break;
    default:
        break;
    }
}


int file_watcher_foo()
{
    _fw = fw_init(on_change);
    if (!_fw) {
        printf("fw_init failed!\n");
        return -1;
    }
    fw_add_watch_recursive(_fw, ROOT_DIR);
    fw_dispatch(_fw);
    return 0;
}
#endif

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
#ifdef ENABLE_FILEWATCHER
    file_watcher_foo();
#endif
    return 0;
}
