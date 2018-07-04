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
#ifndef LIBFILEWATCHER_H
#define LIBFILEWATCHER_H

#include <dirent.h>
#include <libdict.h>
#include <libgevent.h>

#ifdef __cplusplus
extern "C" {
#endif

enum fw_type {
    FW_CREATE_DIR = 0,
    FW_CREATE_FILE,
    FW_DELETE_DIR,
    FW_DELETE_FILE,
    FW_MOVE_FROM_DIR,
    FW_MOVE_TO_DIR,
    FW_MOVE_FROM_FILE,
    FW_MOVE_TO_FILE,
    FW_MODIFY_FILE,
};

typedef struct fw {
    int fd;
    struct gevent_base *evbase;
    dict *dict_path;
    void (*notify_cb)(struct fw *fw, enum fw_type type, char *path);
} fw_t;


struct fw *fw_init(void (notify_cb)(struct fw *fw, enum fw_type type, char *path));
void fw_deinit(struct fw *fw);
int fw_add_watch_recursive(struct fw *fw, const char *path);
int fw_del_watch_recursive(struct fw *fw, const char *path);
int fw_dispatch(struct fw *fw);


#ifdef __cplusplus
}
#endif
#endif
