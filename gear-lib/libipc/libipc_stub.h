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
