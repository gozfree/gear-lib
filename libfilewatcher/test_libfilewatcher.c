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
#include <unistd.h>
#include <signal.h>
#include "libfilewatcher.h"

#define ROOT_DIR		"./"
static struct fw *_fw = NULL;

void ctrl_c_op(int signo)
{
    fw_deinit(_fw);
    printf("file_watcher exit\n");
}

void on_change(struct fw *fw, enum fw_type type, char *path)
{
    switch (type) {
    case FW_CREATE_DIR:
        printf("[CREATE DIR] %s\n", path);
        break;
    case FW_CREATE_FILE:
        printf("[CREATE FILE] %s\n", path);
        break;
    case FW_DELETE_DIR:
        printf("[DELETE DIR] %s\n", path);
        break;
    case FW_DELETE_FILE:
        printf("[DELETE FILE] %s\n", path);
        break;
    case FW_MOVE_FROM_DIR:
        printf("[MOVE FROM DIR] %s\n", path);
        break;
    case FW_MOVE_TO_DIR:
        printf("[MOVE TO DIR] %s\n", path);
        break;
    case FW_MOVE_FROM_FILE:
        printf("[MOVE FROM FILE] %s\n", path);
        break;
    case FW_MOVE_TO_FILE:
        printf("[MOVE TO FILE] %s\n", path);
        break;
    case FW_MODIFY_FILE:
        printf("[MODIFY FILE] %s\n", path);
        break;
    default:
        break;
    }
}


int main(int argc, char **argv)
{
    signal(SIGINT, ctrl_c_op);
    _fw = fw_init(on_change);
    if (!_fw) {
        printf("fw_init failed!\n");
        return -1;
    }
    fw_add_watch_recursive(_fw, ROOT_DIR);
    fw_dispatch(_fw);
    return 0;
}
