/******************************************************************************
 * Copyright (C) 2014-2015
 * file:    librpc_stub.h
 * author:  gozfree <gozfree@163.com>
 * created: 2015-10-31 19:09
 * updated: 2015-10-31 19:09
 ******************************************************************************/
#ifndef _LIBRPC_STUB_H_
#define _LIBRPC_STUB_H_


#include "librpc.h"

enum rpc_cmd {
    _RPC_TEST = _RPC_USER_BASE,
    _RPC_GET_CONNECT_LIST,
    _RPC_PEER_POST_MSG,
    _RPC_SHELL_HELP,
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

#define RPC_CALC \
    BUILD_RPC_MSG_ID(_RPC_GROUP_0, _RPC_NEED_RETURN, _RPC_DIR_UP, _RPC_PARSE_JSON, _RPC_CALC)


#endif
