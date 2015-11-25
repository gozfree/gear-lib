/******************************************************************************
 * Copyright (C) 2014-2015
 * file:    libipc_stub.h
 * author:  gozfree <gozfree@163.com>
 * created: 2015-11-23 02:04
 * updated: 2015-11-23 02:04
 ******************************************************************************/
#ifndef _LIBIPC_STUB_H_
#define _LIBIPC_STUB_H_

#include "libipc.h"

#define IPC_REGISTER_MAP(map_name)             \
    ipc_register_map(__ipc_action_map##map_name,  \
                     (sizeof(__ipc_action_map##map_name )/sizeof(ipc_handler_t)));


#define BEGIN_IPC_MAP(map_name)  \
    static ipc_handler_t  __ipc_action_map##map_name[] = {
#define IPC_ACTION(x, y) {x, y},
#define END_IPC_MAP() };

/*
 *  - 31 .................................. 0
 *  - Bit    31: call direction
 *    - 0: DOWN (server to client)
 *    - 1: UP   (client to server)
 *
 *  - Bit    30: return indicator
 *    - 0: no need return
 *    - 1: need return
 *
 *  - Bit    29: group CMD indicator
 *    - 0: not group CMD
 *    - 1: group CMD
 *  - Bit     0: generic group
 *  - Bit  1~15: group id defined
 *  - Bit 25~28: group CMD id
 *  - Bit 24~16: type
 *  - Bit 15~ 0: CMD id
 */

typedef enum ipc_dir {
    IPC_DIR_UP = 0,
    IPC_DIR_DOWN = 1,
} ipc_dir_t;

typedef enum ipc_parse {
    IPC_PARSE_JSON = 0,
    IPC_PARSE_PROTOBUF = 1,
} ipc_parse_t;

typedef enum ipc_cmd {
    _IPC_TEST             = 0,
    _IPC_GET_CONNECT_LIST = 1,
    _IPC_PEER_POST_MSG    = 2,
    _IPC_SHELL_HELP       = 3,
    _IPC_CALC             = 4,
} ipc_cmd_t;


#define MAX_MESSAGES_IN_MAP         256
#define IPC_DIR_BIT                 24
#define IPC_CMD_BIT                 8
#define IPC_PARSE_BIT               0

#define IPC_TYPE_MASK               0xFFFF
#define IPC_CMD_MASK                0xF
#define IPC_DIR_MASK                0xF
#define IPC_PARSE_MASK              0xF

#define BUILD_IPC_TYPE(dir, cmd, parse) \
    (((((uint32_t)cmd) & IPC_CMD_MASK) << IPC_CMD_BIT) | \
     ((((uint32_t)dir) & IPC_DIR_MASK) << IPC_DIR_BIT) | \
     ((((uint32_t)parse) & IPC_PARSE_MASK) << IPC_PARSE_BIT))

#define GET_IPC_MSG_NEED_RETURN(cmd) \
        (((cmd)>>AM_SYS_IPC_MSG_NEED_RETURN_BIT) & AM_SYS_IPC_MSG_DIRECTION_MASK)


#define IPC_TEST \
    BUILD_IPC_TYPE(IPC_DIR_UP, _IPC_TEST, IPC_PARSE_JSON)

#define IPC_GET_CONNECT_LIST \
    BUILD_IPC_TYPE(IPC_DIR_UP, _IPC_GET_CONNECT_LIST, IPC_PARSE_JSON)

#define IPC_PEER_POST_MSG \
    BUILD_IPC_TYPE(IPC_DIR_UP, _IPC_PEER_POST_MSG, IPC_PARSE_JSON)

#define IPC_SHELL_HELP \
    BUILD_IPC_TYPE(IPC_DIR_UP, _IPC_SHELL_HELP, IPC_PARSE_JSON)

#define IPC_CALC \
    BUILD_IPC_TYPE(IPC_DIR_UP, _IPC_CALC, IPC_PARSE_JSON)



#endif
