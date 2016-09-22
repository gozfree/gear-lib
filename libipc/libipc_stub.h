/******************************************************************************
 * Copyright (C) 2014-2015
 * file:    libipc_stub.h
 * author:  gozfree <gozfree@163.com>
 * created: 2015-11-23 02:04
 * updated: 2015-11-23 02:04
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
