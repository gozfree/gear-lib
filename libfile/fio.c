/******************************************************************************
 * Copyright (C) 2014-2015
 * file:    fio.c
 * author:  gozfree <gozfree@163.com>
 * created: 2016-07-22 16:26
 * updated: 2016-07-22 16:26
 ******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "libfile.h"

#define MAX_RETRY_CNT	(3)
static struct file_desc *fio_open(const char *path, file_open_mode_t mode)
{
    struct file_desc *file = (struct file_desc *)calloc(1,
                             sizeof(struct file_desc));
    if (!file) {
        printf("malloc failed:%d %s\n", errno, strerror(errno));
        return NULL;
    }
    const char *flags = NULL;
    switch(mode) {
      case F_RDONLY:
        flags = "r";
        break;
      case F_WRONLY:
        flags = "w";
        break;
      case F_RDWR:
        flags = "r+";
        break;
      case F_CREATE:
        flags = "w+";
        break;
      case F_WRCLEAR:
        flags = "w+";
        break;
      default:
        printf("unsupport file mode!\n");
        break;
    }

    file->fp = fopen(path, flags);
    if (!file->fp) {
        printf("fopen %s failed:%d %s\n", path, errno, strerror(errno));
        free(file);
        return NULL;
    }
    return file;
}

static ssize_t fio_read(struct file_desc *file, void *buf, size_t len)
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
    FILE *fp = file->fp;
    while (left > 0) {
        if (left < step)
            step = left;
        n = fread((void *)p, 1, step, fp);
        if (n > 0) {
            p += n;
            left -= n;
            continue;
        } else {
            if (0 != feof(fp)) {
                clearerr(fp);
                break;
            } else {
                if (++cnt > MAX_RETRY_CNT)
                    break;
                continue;
            }
        }
    }
    return (len - left);
}

static ssize_t fio_write(struct file_desc *file, const void *buf, size_t count)
{
    ssize_t n;
    char *cursor = (char *)buf;
    int retry = 0;
    if (file == NULL || buf == NULL || count == 0) {
        printf("%s paraments invalid!\n", __func__);
        return -1;
    }
    FILE *fp = file->fp;
    size_t step = 1024 * 1024;
    size_t remain = count;
    while (remain > 0) {
        if (remain < step)
            step = remain;
        n = fwrite((void *)cursor, 1, step, fp);
        if (n > 0) {
            cursor += n;
            remain -= n;
            continue;
        } else {
            if (errno == EINTR || errno == EAGAIN) {
                if (++retry > MAX_RETRY_CNT) {
                    printf("reach max retry\n");
                    break;
                }
                continue;
            } else {
                printf("write failed:%d %s\n", errno, strerror(errno));
                break;
            }
        }
    }
    return (count - remain);
}

static off_t fio_seek(struct file_desc *file, off_t offset, int whence)
{
    if (!file) {
        return -1;
    }
    return fseek(file->fp, offset, whence);
}

static int fio_sync(struct file_desc *file)
{
    if (!file) {
        return -1;
    }
    return fsync(file->fd);
}

static void fio_close(struct file_desc *file)
{
    if (file) {
        fclose(file->fp);
        free(file);
    }
}

struct file_ops fio_ops = {
    .open  = fio_open,
    .write = fio_write,
    .read  = fio_read,
    .seek  = fio_seek,
    .sync  = fio_sync,
    .close = fio_close,
};
