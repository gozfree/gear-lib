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
#ifndef LIBIPC_STUB_H
#define LIBIPC_STUB_H

#include "libipc.h"

enum ipc_cmd {
    _IPC_TEST = _IPC_USER_BASE,
    _IPC_GET_CONNECT_LIST,
    _IPC_SHELL_HELP,
    _IPC_CALC,
    _IPC_USER_MAX   = 255
};

enum ipc_group {
    _IPC_GROUP_0    = 0,
    _IPC_GROUP_1,
    _IPC_GROUP_2,
};

#define IPC_TEST \
    BUILD_IPC_MSG_ID(_IPC_GROUP_0, _IPC_NO_RETURN, _IPC_DIR_UP, _IPC_PARSE_JSON, _IPC_TEST)

#define IPC_GET_CONNECT_LIST \
    BUILD_IPC_MSG_ID(_IPC_GROUP_0, _IPC_NO_RETURN, _IPC_DIR_UP, _IPC_PARSE_JSON, _IPC_GET_CONNECT_LIST)

#define IPC_SHELL_HELP \
    BUILD_IPC_MSG_ID(_IPC_GROUP_0, _IPC_NEED_RETURN, _IPC_DIR_UP, _IPC_PARSE_JSON, _IPC_SHELL_HELP)

#define IPC_CALC \
    BUILD_IPC_MSG_ID(_IPC_GROUP_0, _IPC_NEED_RETURN, _IPC_DIR_UP, _IPC_PARSE_JSON, _IPC_CALC)


#endif
