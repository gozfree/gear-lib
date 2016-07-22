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
#include "libmacro.h"

extern const struct file_ops io_ops;
extern const struct file_ops fio_ops;

typedef enum file_backend_type {
    FILE_BACKEND_IO,
    FILE_BACKEND_FIO,
} ipc_backend_type;


static const struct file_ops *file_ops[] = {
    &io_ops,
    &fio_ops,
    NULL
};



struct file *file_open(const char *path, file_open_mode_t mode)
{
    struct file *file = CALLOC(1, struct file);
    if (!file) {
        printf("malloc failed!\n");
        return NULL;
    }
    file->ops = file_ops[FILE_BACKEND_IO];
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


ssize_t file_size(const char *path)
{
  struct stat st;
  off_t size = 0;
  if (stat(path, &st) < 0) {
    printf("%s stat error: %s", path, strerror(errno));
  } else {
    size = st.st_size;
  }

  return size;
}

