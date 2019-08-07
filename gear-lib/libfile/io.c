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
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#if defined (__linux__) || defined (__CYGWIN__)
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#endif

#define MAX_RETRY_CNT   (3)

static struct file_desc *io_open(const char *path, file_open_mode_t mode)
{
    int flags = -1;
    struct file_desc *file = (struct file_desc *)calloc(1,
                             sizeof(struct file_desc));
    if (!file) {
        printf("malloc failed:%d %s\n", errno, strerror(errno));
        return NULL;
    }
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
    int fd;
    char *p = (char *)buf;
    size_t left = len;
    size_t step = 1024*1024;
    int cnt = 0;
    if (file == NULL || buf == NULL || len == 0) {
        printf("%s paraments invalid!\n", __func__);
        return -1;
    }
    fd = file->fd;
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
    int fd;
    char *p = (char *)buf;
    size_t left = len;
    size_t step = 1024 * 1024;
    int cnt = 0;
    if (file == NULL || buf == NULL || len == 0) {
        printf("%s paraments invalid\n", __func__);
        return -1;
    }
    fd = file->fd;
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
#if defined (__linux__) || defined (__CYGWIN__)
    return fsync(file->fd);
#else
    return 0;
#endif
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
    io_open,
    io_write,
    io_read,
    io_seek,
    io_sync,
    io_size,
    io_close,
};
