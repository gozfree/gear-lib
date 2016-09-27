/******************************************************************************
 * Copyright (C) 2014-2015
 * file:    libfile.h
 * author:  gozfree <gozfree@163.com>
 * created: 2016-07-22 14:17:17
 * updated: 2016-07-22 14:17:17
 *****************************************************************************/
#ifndef LIBFILE_H
#define LIBFILE_H

#include <stdint.h>
#include <sys/uio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum file_open_mode {
    F_RDONLY,
    F_WRONLY,
    F_RDWR,
    F_CREATE,
    F_WRCLEAR,
} file_open_mode_t;

struct file_desc {
    union {
        int fd;
        FILE *fp;
    };
    char *name;
};

typedef struct file {
    struct file_desc *fd;
    const struct file_ops *ops;
    uint64_t size;
} file;

typedef struct file_ops {
    struct file_desc * (*open)(const char *path, file_open_mode_t mode);
    ssize_t (*write)(struct file_desc *fd, const void *buf, size_t count);
    ssize_t (*read)(struct file_desc *fd, void *buf, size_t count);
    off_t (*seek)(struct file_desc *fd, off_t offset, int whence);
    int (*sync)(struct file_desc *fd);
    size_t (*size)(struct file_desc *fd);
    void (*close)(struct file_desc *fd);
} file_ops_t;

typedef enum file_backend_type {
    FILE_BACKEND_IO,
    FILE_BACKEND_FIO,
} file_backend_type;

void file_backend(file_backend_type type);
struct file *file_open(const char *path, file_open_mode_t mode);
void file_close(struct file *file);
ssize_t file_read(struct file *file, void *data, size_t size);
ssize_t file_write(struct file *file, const void *data, size_t size);
ssize_t file_size(struct file *file);
ssize_t file_get_size(const char *path);
struct iovec *file_dump(const char *path);
int file_sync(struct file *file);
off_t file_seek(struct file *file, off_t offset, int whence);

#ifdef __cplusplus
}
#endif
#endif
