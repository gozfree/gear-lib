/******************************************************************************
 * Copyright (C) 2014-2015
 * file:    libfile.c
 * author:  gozfree <gozfree@163.com>
 * created: 2016-07-22 14:17:17
 * updated: 2016-07-22 14:17:17
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "libfile.h"

extern const struct file_ops io_ops;
extern const struct file_ops fio_ops;


static const struct file_ops *file_ops[] = {
    &io_ops,
    &fio_ops,
    NULL
};

static file_backend_type backend = FILE_BACKEND_IO;

void file_backend(file_backend_type type)
{
    backend = type;
}

struct file *file_open(const char *path, file_open_mode_t mode)
{
    struct file *file = (struct file *)calloc(1, sizeof(struct file));
    if (!file) {
        printf("malloc failed!\n");
        return NULL;
    }
    file->ops = file_ops[backend];
    file->fd = file->ops->open(path, mode);
    return file;
}

void file_close(struct file *file)
{
    return file->ops->close(file->fd);
}

ssize_t file_read(struct file *file, void *data, size_t size)
{
    return file->ops->read(file->fd, data, size);
}

ssize_t file_write(struct file *file, const void *data, size_t size)
{
    return file->ops->write(file->fd, data, size);
}

ssize_t file_size(struct file *file)
{
    return file->ops->size(file->fd);
}

int file_sync(struct file *file)
{
    return file->ops->sync(file->fd);
}

off_t file_seek(struct file *file, off_t offset, int whence)
{
    return file->ops->seek(file->fd, offset, whence);
}

ssize_t file_get_size(const char *path)
{
    struct stat st;
    off_t size = 0;
    if (stat(path, &st) < 0) {
        printf("%s stat error: %s\n", path, strerror(errno));
    } else {
        size = st.st_size;
    }
    return size;
}

struct iovec *file_dump(const char *path)
{
    ssize_t size = file_get_size(path);
    if (size == 0) {
        return NULL;
    }
    struct iovec *buf = (struct iovec *)calloc(1, sizeof(struct iovec));
    if (!buf) {
        printf("malloc failed!\n");
        return NULL;
    }
    buf->iov_len = size;
    buf->iov_base = calloc(1, buf->iov_len);
    if (!buf->iov_base) {
        printf("malloc failed!\n");
        return NULL;
    }

    struct file *f = file_open(path, F_RDONLY);
    if (!f) {
        printf("file open failed!\n");
        free(buf->iov_base);
        free(buf);
        return NULL;
    }
    file_read(f, buf->iov_base, buf->iov_len);
    file_close(f);
    return buf;
}

