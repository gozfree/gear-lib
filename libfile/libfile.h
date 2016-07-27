/******************************************************************************
 * Copyright (C) 2014-2015
 * file:    libfile.h
 * author:  gozfree <gozfree@163.com>
 * created: 2016-07-22 14:17:17
 * updated: 2016-07-22 14:17:17
 *****************************************************************************/
#ifndef _LIBFILE_H_
#define _LIBFILE_H_

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
    void (*close)(struct file_desc *fd);
} file_ops_t;

struct file *file_open(const char *path, file_open_mode_t mode);
void file_close(struct file *file);
ssize_t file_read(struct file *file, void *data, size_t size);
ssize_t file_write(struct file *file, const void *data, size_t size);
ssize_t file_size(const char *path);
struct iovec *file_dump(const char *path);


#ifdef __cplusplus
}
#endif
#endif
