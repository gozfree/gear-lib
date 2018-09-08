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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#define MAX_RETRY_CNT   (3)

static struct file_desc *io_open(const char *path, file_open_mode_t mode)
{
    struct file_desc *file = (struct file_desc *)calloc(1,
                             sizeof(struct file_desc));
    if (!file) {
        printf("malloc failed:%d %s\n", errno, strerror(errno));
        return NULL;
    }
    int flags = -1;
    switch(mode) {
    case F_RDONLY:
        flags = O_RDONLY;
        break;
    case F_WRONLY:
        flags = O_WRONLY;
        break;
    case F_RDWR:
        flags = O_RDWR;
        break;
    case F_CREATE:
        flags = O_RDWR|O_TRUNC|O_CREAT;
        break;
    case F_WRCLEAR:
        flags = O_WRONLY|O_TRUNC|O_CREAT;
        break;
    case F_APPEND:
        flags = O_RDWR|O_APPEND;
        break;
    default:
        printf("unsupport file mode!\n");
        break;
    }
    file->fd = open(path, flags, 0666);
    if (file->fd == -1) {
        printf("open %s failed:%d %s\n", path, errno, strerror(errno));
        free(file);
        return NULL;
    }
    file->name = strdup(path);
    return file;
}

static ssize_t io_read(struct file_desc *file, void *buf, size_t len)
{
    int n;
    char *p = (char *)buf;
    size_t left = len;
    size_t step = 1024*1024;
    int cnt = 0;
    if (file == NULL || buf == NULL || len == 0) {
        printf("%s paraments invalid!\n", __func__);
        return -1;
    }
    int fd = file->fd;
    while (left > 0) {
        if (left < step)
            step = left;
        n = read(fd, (void *)p, step);
        if (n > 0) {
            p += n;
            left -= n;
            continue;
        } else if (n == 0) {
            break;
        }
        if (errno == EINTR || errno == EAGAIN) {
            if (++cnt > MAX_RETRY_CNT) {
                printf("reach max retry count\n");
                break;
            }
            continue;
        } else {
            printf("read failed: %d\n", errno);
            break;
        }
    }
    return (len - left);
}

static ssize_t io_write(struct file_desc *file, const void *buf, size_t len)
{
    ssize_t n;
    char *p = (char *)buf;
    size_t left = len;
    size_t step = 1024 * 1024;
    int cnt = 0;
    if (file == NULL || buf == NULL || len == 0) {
        printf("%s paraments invalid, ", __func__);
        return -1;
    }
    int fd = file->fd;
    while (left > 0) {
        if (left < step)
            step = left;
        n = write(fd, (void *)p, step);
        if (n > 0) {
            p += n;
            left -= n;
            continue;
        } else if (n == 0) {
            break;
        }
        if (errno == EINTR || errno == EAGAIN) {
            if (++cnt > MAX_RETRY_CNT) {
                printf("reach max retry count\n");
                break;
            }
            continue;
        } else {
            printf("write failed: %d\n", errno);
            break;
        }
    }
    return (len - left);
}

static off_t io_seek(struct file_desc *file, off_t offset, int whence)
{
    if (!file) {
        return -1;
    }
    return lseek(file->fd, offset, whence);
}

static int io_sync(struct file_desc *file)
{
    if (!file) {
        return -1;
    }
    return fsync(file->fd);
}

static size_t io_size(struct file_desc *file)
{
    struct stat buf;
    if (stat(file->name, &buf) < 0) {
        return 0;
    }
    return (size_t)buf.st_size;
}

static void io_close(struct file_desc *file)
{
    if (file) {
        close(file->fd);
        free(file->name);
        free(file);
    }
}

struct file_ops io_ops = {
    .open  = io_open,
    .write = io_write,
    .read  = io_read,
    .seek  = io_seek,
    .sync  = io_sync,
    .size  = io_size,
    .close = io_close,
};
