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
#ifndef LIBFILE_H
#define LIBFILE_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
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

typedef struct file_info {
    struct timespec time_modify;
    struct timespec time_access;
    uint64_t size;
} file_info;

typedef struct file_systat {
    uint64_t size_total;
    uint64_t size_avail;
    uint64_t size_free;
    char fs_type_name[32];
} file_systat;

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
int file_create(const char *path);
void file_delete(const char *path);
bool file_exist(const char *path);
struct file *file_open(const char *path, file_open_mode_t mode);
void file_close(struct file *file);
ssize_t file_read(struct file *file, void *data, size_t size);
ssize_t file_read_path(const char *path, void *data, size_t size);
ssize_t file_write(struct file *file, const void *data, size_t size);
ssize_t file_write_path(const char *path, const void *data, size_t size);
ssize_t file_size(struct file *file);
ssize_t file_get_size(const char *path);
struct iovec *file_dump(const char *path);
int file_sync(struct file *file);
off_t file_seek(struct file *file, off_t offset, int whence);
struct file_systat *file_get_systat(const char *path);
char *file_path_pwd();
char *file_path_suffix(char *path);
char *file_path_prefix(char *path);

int file_dir_create(const char *path);
int file_dir_remove(const char *path);
int file_dir_tree(const char *path);
int file_dir_size(const char *path, uint64_t *size);
int file_num_in_dir(const char *path);
#ifdef __cplusplus
}
#endif
#endif
