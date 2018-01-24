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
#ifndef LIBRPC_STUB_H
#define LIBRPC_STUB_H


#include "librpc.h"

enum rpc_cmd {
    _RPC_TEST = _RPC_USER_BASE,
    _RPC_GET_CONNECT_LIST,
    _RPC_PEER_POST_MSG,
    _RPC_SHELL_HELP,
    _RPC_HELLO,
    _RPC_CALC,
    _RPC_USER_MAX   = 255
};

enum rpc_group {
    _RPC_GROUP_0    = 0,
    _RPC_GROUP_1,
    _RPC_GROUP_2,
};

#define RPC_TEST \
    BUILD_RPC_MSG_ID(_RPC_GROUP_0, _RPC_NO_RETURN, _RPC_DIR_UP, _RPC_PARSE_JSON, _RPC_TEST)

#define RPC_GET_CONNECT_LIST \
    BUILD_RPC_MSG_ID(_RPC_GROUP_0, _RPC_NEED_RETURN, _RPC_DIR_UP, _RPC_PARSE_JSON, _RPC_GET_CONNECT_LIST)

#define RPC_SHELL_HELP \
    BUILD_RPC_MSG_ID(_RPC_GROUP_0, _RPC_NEED_RETURN, _RPC_DIR_UP, _RPC_PARSE_JSON, _RPC_SHELL_HELP)

#define RPC_PEER_POST_MSG \
    BUILD_RPC_MSG_ID(_RPC_GROUP_0, _RPC_NO_RETURN, _RPC_DIR_UP, _RPC_PARSE_JSON, _RPC_PEER_POST_MSG)

#define RPC_HELLO \
    BUILD_RPC_MSG_ID(_RPC_GROUP_0, _RPC_NEED_RETURN, _RPC_DIR_UP, _RPC_PARSE_PROTOBUF, _RPC_HELLO)


#define RPC_CALC \
    BUILD_RPC_MSG_ID(_RPC_GROUP_0, _RPC_NEED_RETURN, _RPC_DIR_UP, _RPC_PARSE_JSON, _RPC_CALC)


#endif
