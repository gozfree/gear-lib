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
