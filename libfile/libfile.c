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
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <libgen.h>
#include <limits.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include "libfile.h"

/*
 * Most of these MAGIC constants are defined in /usr/include/linux/magic.h,
 * and some are hardcoded in kernel sources.
 */
typedef enum fs_type_supported {
    FS_CIFS     = 0xFF534D42,
    FS_CRAMFS   = 0x28cd3d45,
    FS_DEBUGFS  = 0x64626720,
    FS_DEVFS    = 0x1373,
    FS_DEVPTS   = 0x1cd1,
    FS_EXT      = 0x137D,
    FS_EXT2_OLD = 0xEF51,
    FS_EXT2     = 0xEF53,
    FS_EXT3     = 0xEF53,
    FS_EXT4     = 0xEF53,
    FS_FUSE     = 0x65735546,
    FS_JFFS2    = 0x72b6,
    FS_MQUEUE   = 0x19800202,
    FS_MSDOS    = 0x4d44,
    FS_NFS      = 0x6969,
    FS_NTFS     = 0x5346544e,
    FS_PROC     = 0x9fa0,
    FS_RAMFS    = 0x858458f6,
    FS_ROMFS    = 0x7275,
    FS_SELINUX  = 0xf97cff8c,
    FS_SMB      = 0x517B,
    FS_SOCKFS   = 0x534F434B,
    FS_SQUASHFS = 0x73717368,
    FS_SYSFS    = 0x62656572,
    FS_TMPFS    = 0x01021994
} fs_type_supported_t;

static struct {
    const char name[32];
    const int value;
} fs_type_info[] = {
    {"CIFS    ", FS_CIFS    },
    {"CRAMFS  ", FS_CRAMFS  },
    {"DEBUGFS ", FS_DEBUGFS },
    {"DEVFS   ", FS_DEVFS   },
    {"DEVPTS  ", FS_DEVPTS  },
    {"EXT     ", FS_EXT     },
    {"EXT2_OLD", FS_EXT2_OLD},
    {"EXT2    ", FS_EXT2    },
    {"EXT3    ", FS_EXT3    },
    {"EXT4    ", FS_EXT4    },
    {"FUSE    ", FS_FUSE    },
    {"JFFS2   ", FS_JFFS2   },
    {"MQUEUE  ", FS_MQUEUE  },
    {"MSDOS   ", FS_MSDOS   },
    {"NFS     ", FS_NFS     },
    {"NTFS    ", FS_NTFS    },
    {"PROC    ", FS_PROC    },
    {"RAMFS   ", FS_RAMFS   },
    {"ROMFS   ", FS_ROMFS   },
    {"SELINUX ", FS_SELINUX },
    {"SMB     ", FS_SMB     },
    {"SOCKFS  ", FS_SOCKFS  },
    {"SQUASHFS", FS_SQUASHFS},
    {"SYSFS   ", FS_SYSFS   },
    {"TMPFS   ", FS_TMPFS   },
};

extern const struct file_ops io_ops;
extern const struct file_ops fio_ops;


static const struct file_ops *file_ops[] = {
    &io_ops,
    &fio_ops,
    NULL
};

#define SIZEOF(array)       (sizeof(array)/sizeof(array[0]))

static file_backend_type backend = FILE_BACKEND_IO;
static char local_path[PATH_MAX];

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
    if (!file || !file->ops) {
        return;
    }
    file->ops->close(file->fd);
    free(file);
}

ssize_t file_read(struct file *file, void *data, size_t size)
{
    if (!file || !data || size == 0) {
        return -1;
    }
    return file->ops->read(file->fd, data, size);
}

ssize_t file_read_path(const char *path, void *data, size_t size)
{
    struct file *fp = file_open(path, F_RDONLY);
    if (!fp) {
        return -1;
    }
    ssize_t flen = file_read(fp, data, size);
    file_close(fp);
    return flen;
}

ssize_t file_write(struct file *file, const void *data, size_t size)
{
    if (!file || !data || size == 0) {
        return -1;
    }
    return file->ops->write(file->fd, data, size);
}

ssize_t file_write_path(const char *path, const void *data, size_t size)
{
    struct file *fp = file_open(path, F_WRCLEAR);
    if (!fp) {
        return -1;
    }
    ssize_t flen = file_write(fp, data, size);
    file_close(fp);
    return flen;
}

ssize_t file_size(struct file *file)
{
    if (!file) {
        return -1;
    }
    return file->ops->size(file->fd);
}

int file_sync(struct file *file)
{
    if (!file) {
        return -1;
    }
    return file->ops->sync(file->fd);
}

off_t file_seek(struct file *file, off_t offset, int whence)
{
    if (!file) {
        return -1;
    }
    return file->ops->seek(file->fd, offset, whence);
}

ssize_t file_get_size(const char *path)
{
    if (!path) {
        return -1;
    }
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
    if (!path) {
        return NULL;
    }
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


struct file_systat *file_get_systat(const char *path)
{
    if (!path) {
        printf("path can't be null\n");
        return NULL;
    }
    int i;
    struct statfs stfs;
    if (-1 == statfs(path, &stfs)) {
        printf("statfs %s failed: %s\n", path, strerror(errno));
        return NULL;
    }
    struct file_systat *fi = (struct file_systat *)calloc(1,
                    sizeof(struct file_systat));
    if (!fi) {
        printf("malloc failed!\n");
        return NULL;
    }
    fi->size_total = stfs.f_bsize * stfs.f_blocks;
    fi->size_avail = stfs.f_bsize * stfs.f_bavail;
    fi->size_free  = stfs.f_bsize * stfs.f_bfree;
    for (i = 0; i < SIZEOF(fs_type_info); i++) {
        if (stfs.f_type == fs_type_info[i].value) {
            stfs.f_type = i;
            strncpy(fi->fs_type_name, fs_type_info[i].name,
                            sizeof(fi->fs_type_name));
            break;
        }
    }
    return fi;
}

char *file_path_pwd()
{
    char *tmp = getcwd(local_path, sizeof(local_path));
    if (!tmp) {
        printf("getcwd failed: %s\n", strerror(errno));
    }
    return tmp;
}

char *file_path_suffix(char *path)
{
    return basename(path);
}

char *file_path_prefix(char *path)
{
    return dirname(path);
}

bool file_exist(const char *path)
{
    return (access(path, F_OK|W_OK|R_OK) == 0) ? true : false;
}

static int mkdir_r(const char *path, mode_t mode)
{
    if (!path) {
        return -1;
    }
    char *temp = strdup(path);
    char *pos = temp;
    int ret = 0;

    if (strncmp(temp, "/", 1) == 0) {
        pos += 1;
    } else if (strncmp(temp, "./", 2) == 0) {
        pos += 2;
    }
    for ( ; *pos != '\0'; ++ pos) {
        if (*pos == '/') {
            *pos = '\0';
            if (-1 == (ret = mkdir(temp, mode))) {
                if (errno == EEXIST) {
                    ret = 0;
                } else {
                    fprintf(stderr, "failed to mkdir %s: %d:%s\n",
                                    temp, errno, strerror(errno));
                    break;
                }
            }
            *pos = '/';
        }
    }
    if (*(pos - 1) != '/') {
        if (-1 == (ret = mkdir(temp, mode))) {
            if (errno == EEXIST) {
                ret = 0;
            } else {
                fprintf(stderr, "failed to mkdir %s: %d:%s\n",
                                temp, errno, strerror(errno));
            }
        }
    }
    free(temp);
    return ret;
}

int file_dir_create(const char *path)
{
    return mkdir_r(path, 0775);
}

int dfs_remove_dir(const char *path)
{
    DIR *pdir = NULL;
    struct dirent *ent = NULL;
    char full_path[PATH_MAX];
    int ret = 0;
    pdir = opendir(path);
    if (!pdir) {
        printf("can not open path: %s\n", path);
        if (errno == EMFILE) {
            return -EMFILE;
        } else {
            return -1;
        }
    }
    while (NULL != (ent = readdir(pdir))) {
        if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..")) {
            continue;
        }
        memset(full_path, 0, sizeof(full_path));
        sprintf(full_path, "%s/%s", path, ent->d_name);
        if (ent->d_type == DT_DIR) {
            ret = dfs_remove_dir(full_path);
            if (ret != 0) {
                printf("dfs_remove_dir %s ret=%d\n", full_path, ret);
            }
        }
        ret = remove(full_path);
    }
    closedir(pdir);
    return ret;
}

int file_dir_remove(const char *path)
{
    dfs_remove_dir(path);
    return remove(path);
}

int file_dir_tree(const char *path)
{
    DIR *pdir = NULL;
    struct dirent *ent = NULL;
    char full_path[PATH_MAX];
    int ret;
    pdir = opendir(path);
    if (!pdir) {
        printf("can not open path: %s\n", path);
        if (errno == EMFILE) {
            return -EMFILE;
        } else {
            return -1;
        }
    }
    while (NULL != (ent = readdir(pdir))) {
        memset(full_path, 0, sizeof(full_path));
        sprintf(full_path, "%s/%s", path, ent->d_name);
        if (ent->d_type == DT_DIR) {
            if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..")) {
                continue;
            }

            ret = file_dir_tree(full_path);
            if (ret == -EMFILE) {
                closedir(pdir);
                return ret;
            }
        }
    }
    closedir(pdir);
    return 0;
}

int dfs_dir_size(const char *path, uint64_t *size)
{
    DIR *pdir = NULL;
    struct dirent *ent = NULL;
    char full_path[PATH_MAX];
    int ret;
    pdir = opendir(path);
    if (!pdir) {
        printf("can not open path: %s\n", path);
        if (errno == EMFILE) {
            return -EMFILE;
        } else {
            return -1;
        }
    }
    while (NULL != (ent = readdir(pdir))) {
        memset(full_path, 0, sizeof(full_path));
        sprintf(full_path, "%s/%s", path, ent->d_name);
        if (ent->d_type == DT_DIR) {
            if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..")) {
                continue;
            }
            ret = dfs_dir_size(full_path, size);
            if (ret == -EMFILE) {
                closedir(pdir);
                return ret;
            }
        } else if (ent->d_type == DT_REG) {
            *size += file_get_size(full_path);
        }
    }
    closedir(pdir);
    return 0;
}

int file_dir_size(const char *path, uint64_t *size)
{
    *size = 0;
    return dfs_dir_size(path, size);
}

int file_num_in_dir(const char *path)
{
    if (!path) {
        return -1;
    }
    DIR *dir = NULL;
    struct dirent *ent = NULL;
    int num = 0;
    dir = opendir(path);
    if (dir == NULL) {
        return -1;
    }
    while (NULL != (ent = readdir(dir))) {
        if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..")) {
            continue;
        }
        num++;
    }
    closedir(dir);
    return num;
}
