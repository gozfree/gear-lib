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
#ifndef LIBFILE_H
#define LIBFILE_H

#include <stdio.h>
#include <stdint.h>
#include <limits.h>

#if defined (__linux__) || defined (__CYGWIN__)
#include <stdbool.h>
#include <sys/uio.h>
#elif defined (__WIN32__) || defined (WIN32) || defined (_MSC_VER)
#include "libposix4win.h"
#endif


#ifdef __cplusplus
extern "C" {
#endif

typedef enum file_open_mode {
    F_RDONLY,
    F_WRONLY,
    F_RDWR,
    F_CREATE,
    F_WRCLEAR,
    F_APPEND,
} file_open_mode_t;

typedef enum file_type {
    F_NORMAL,
    F_DIR,
    F_LINK,
    F_SOCKET,
    F_DEVICE,

} file_type_t;

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
    uint64_t modify_sec;
    uint64_t access_sec;
    enum file_type type;
    char path[PATH_MAX];
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
int file_delete(const char *path);
bool file_exist(const char *path);

struct file *file_open(const char *path, file_open_mode_t mode);
void file_close(struct file *file);
ssize_t file_read(struct file *file, void *data, size_t size);
ssize_t file_read_path(const char *path, void *data, size_t size);
ssize_t file_write(struct file *file, const void *data, size_t size);
ssize_t file_write_path(const char *path, const void *data, size_t size);
ssize_t file_size(struct file *file);
ssize_t file_get_size(const char *path);
int file_get_info(const char *path, struct file_info *info);
struct iovec *file_dump(const char *path);
int file_sync(struct file *file);
off_t file_seek(struct file *file, off_t offset, int whence);
int file_rename(const char* old_file, const char* new_file);

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
