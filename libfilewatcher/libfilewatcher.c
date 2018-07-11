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
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/inotify.h>
#include <libmacro.h>
#include "libfilewatcher.h"

#define WATCH_MOVED     1
#define WATCH_MODIFY    1
#define KEY_LEN         9

int add_path_list(struct fw *fw, int wd, const char *path)
{
    char key[KEY_LEN];
    memset(key, 0, sizeof(key));
    snprintf(key, KEY_LEN, "%d", wd);
    char *val = CALLOC(PATH_MAX, char);
    strncpy(val, path, PATH_MAX);
    dict_add(fw->dict_path, key, val);
    return 0;
}

int del_path_list(struct fw *fw, int wd)
{
    char key[KEY_LEN];
    memset(key, 0, sizeof(key));
    snprintf(key, sizeof(key), "%d", wd);
    dict_del(fw->dict_path, key);
    return 0;
}

int fw_add_watch(struct fw *fw, const char *path, uint32_t mask)
{
    int fd = fw->fd;
    int wd = inotify_add_watch(fd, path, mask);
    if (wd == -1) {
        printf("inotify_add_watch %s failed(%d): %s\n", path, errno, strerror(errno));
        if (errno == ENOSPC) {
            return -2;
        }
        return -1;
    }
    add_path_list(fw, wd, path);
    return 0;
}

int fw_del_watch(struct fw *fw, const char *path)
{
    int fd = fw->fd;
    int rank = 0;
    char *key, *val;
    while (1) {
        rank = dict_enumerate(fw->dict_path, rank, &key, &val);
        if (rank < 0) {
            return -1;
        }
        if (!strncmp(path, val, strlen(path))) {
            break;
        }
    }
    int wd = atoi(key);
    if (-1 == inotify_rm_watch(fd, wd)) {
        //printf("inotify_rm_watch %d failed(%d):%s\n", wd, errno, strerror(errno));
    }
    del_path_list(fw, wd);
    return 0;
}

int fw_add_watch_recursive(struct fw *fw, const char *path)
{
    int res;
    int fd = fw->fd;
    struct dirent *ent = NULL;
    DIR *pdir = NULL;
    char full_path[PATH_MAX];
    uint32_t mask = IN_CREATE | IN_DELETE;
#if WATCH_MOVED
    mask |= IN_MOVE | IN_MOVE_SELF;
#endif
#if WATCH_MODIFY
    mask |= IN_MODIFY;
#endif
    if (fd == -1 || path == NULL) {
        printf("invalid paraments\n");
        return -1;
    }
    pdir = opendir(path);
    if (!pdir) {
        printf("opendir %s failed(%d): %s\n", path, errno, strerror(errno));
        if (errno == EMFILE) {
            return -2;//stop recursive
        } else {
            return -1;//continue recursive
        }
    }

    while (NULL != (ent = readdir(pdir))) {
        if (ent->d_type == DT_DIR) {
            if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, ".."))
                continue;
            sprintf(full_path, "%s/%s", path, ent->d_name);
            res = fw_add_watch_recursive(fw, full_path);
            if (res == -2) {
                return -2;
            }
        } else if (ent->d_type == DT_REG){
            sprintf(full_path, "%s/%s", path, ent->d_name);
            res = fw_add_watch(fw, full_path, mask);
            if (res == -2) {
                return -2;
            }
        }
    }
    fw_add_watch(fw, path, mask);
    closedir(pdir);
    return 0;
}

int fw_del_watch_recursive(struct fw *fw, const char *path)
{
    int rank = 0;
    char *key, *val;
    while (1) {
        rank = dict_enumerate(fw->dict_path, rank, &key, &val);
        if (rank < 0) {
            break;
        }
        if (!strncmp(path, val, strlen(path))) {
            fw_del_watch(fw, path);
        }
    }
    return 0;
}

int fw_update_watch(struct fw *fw, struct inotify_event *iev)
{
    char full_path[PATH_MAX];
    uint32_t mask = IN_CREATE | IN_DELETE;
#if WATCH_MOVED
    mask |= IN_MOVE | IN_MOVE_SELF;
#endif
#if WATCH_MODIFY
    mask |= IN_MODIFY;
#endif
    char key[KEY_LEN];
    memset(key, 0, sizeof(key));
    snprintf(key, KEY_LEN, "%d", iev->wd);
    char *path = (char *)dict_get(fw->dict_path, key, NULL);
    if (!path) {
        printf("dict_get NULL key=%s\n", key);
        return -1;
    }
    memset(full_path, 0, sizeof(full_path));
    sprintf(full_path, "%s/%s", path, iev->name);
    if (iev->mask & IN_CREATE) {
        if (iev->mask & IN_ISDIR) {
            fw_add_watch_recursive(fw, full_path);
            if (fw->notify_cb)
                fw->notify_cb(fw, FW_CREATE_DIR, full_path);
        } else {
            fw_add_watch(fw, full_path, mask);
            if (fw->notify_cb)
                fw->notify_cb(fw, FW_CREATE_FILE, full_path);
        }
    } else if (iev->mask & IN_DELETE) {
        if (iev->mask & IN_ISDIR) {
            fw_del_watch_recursive(fw, full_path);
            if (fw->notify_cb)
                fw->notify_cb(fw, FW_DELETE_DIR, full_path);
        } else {
            fw_del_watch(fw, full_path);
            if (fw->notify_cb)
                fw->notify_cb(fw, FW_DELETE_FILE, full_path);
        }
    } else if (iev->mask & IN_MOVED_FROM){
        if (iev->mask & IN_ISDIR) {
            fw_del_watch_recursive(fw, full_path);
            if (fw->notify_cb)
                fw->notify_cb(fw, FW_MOVE_FROM_DIR, full_path);
        } else {
            fw_del_watch(fw, full_path);
            if (fw->notify_cb)
                fw->notify_cb(fw, FW_MOVE_FROM_FILE, full_path);
        }
    } else if (iev->mask & IN_MOVED_TO){
        if (iev->mask & IN_ISDIR) {
            fw_add_watch_recursive(fw, full_path);
            if (fw->notify_cb)
                fw->notify_cb(fw, FW_MOVE_TO_DIR, full_path);
        } else {
            fw_add_watch(fw, full_path, mask);
            if (fw->notify_cb)
                fw->notify_cb(fw, FW_MOVE_TO_FILE, full_path);
        }
    } else if (iev->mask & IN_IGNORED){
    } else if (iev->mask & IN_MODIFY){
        if (fw->notify_cb)
            fw->notify_cb(fw, FW_MODIFY_FILE, full_path);
    } else {
        printf("unknown inotify_event:%d\n", iev->mask);
    }
    return 0;
}

struct fw *fw_init(void (notify_cb)(struct fw *fw, enum fw_type type, char *path))
{
    struct fw *fw = CALLOC(1, struct fw);
    if (!fw) {
        printf("malloc fw failed\n");
        goto err;
    }
    int fd = inotify_init();
    if (fd == -1) {
        printf("inotify_init failed: %d\n", errno);
        goto err;
    }
    struct gevent_base *evbase = gevent_base_create();
    if (!evbase) {
        printf("gevent_base_create failed\n");
        goto err;
    }
    fw->fd = fd;
    fw->evbase = evbase;
    fw->dict_path = dict_new();
    fw->notify_cb = notify_cb;
    return fw;
err:
    if (fw) {
        free(fw);
    }
    return NULL;
}

void fw_deinit(struct fw *fw)
{
    if (!fw) {
        return;
    }
    int rank = 0;
    char *key, *val;
    while (1) {
        rank = dict_enumerate(fw->dict_path, rank, &key, &val);
        if (rank < 0) {
            break;
        }
        inotify_rm_watch(fw->fd, atoi(key));
        free(val);
    }
    dict_free(fw->dict_path);
    gevent_base_loop_break(fw->evbase);
    close(fw->fd);
    free(fw);
}

void on_read_ops(int fd, void *arg)
{
#define INOTIFYEVENTBUFSIZE (1024)
    int i, len;
    struct inotify_event *iev;
    size_t iev_size;
    char ibuf[INOTIFYEVENTBUFSIZE] __attribute__ ((aligned(4))) = {0};
    struct fw *fw = (struct fw *)arg;

again:
    len = read(fd, ibuf, sizeof(ibuf));
    if (len == 0) {
        printf("read inofity event buffer 0, error: %s\n", strerror(errno));
    } else if (len == -1) {
        printf("read inofity event buffer error: %s\n", strerror(errno));
        if (errno == EINTR) {
            goto again;
        }
    } else {
        i = 0;
        while (i < len) {
            iev = (struct inotify_event *)(ibuf + i);
            if (iev->len > 0) {
                fw_update_watch(fw, iev);
            } else {
            }
            iev_size = sizeof(struct inotify_event) + iev->len;
            i += iev_size;
        }
    }
}

int fw_dispatch(struct fw *fw)
{
    int fd = fw->fd;
    struct gevent *ev;
    struct gevent_base *evbase = fw->evbase;
    ev = gevent_create(fd, on_read_ops, NULL, NULL, fw);
    gevent_add(evbase, ev);
    gevent_base_loop(evbase);
    return 0;
}
