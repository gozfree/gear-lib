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

#define REGISTER_MSG_MAP(map_name)             \
    register_msg_map(__msg_action_map##map_name,  \
                     (sizeof(__msg_action_map##map_name )/sizeof(msg_handler_t)));


#define BEGIN_MSG_MAP(map_name)  \
    static msg_handler_t  __msg_action_map##map_name[] = {
#define MSG_ACTION(x, y) {x, y},
#define END_MSG_MAP() };


typedef enum rpc_dir {
    RPC_DIR_UP = 0,
    RPC_DIR_DOWN = 1,
} rpc_dir_t;

typedef enum rpc_parse {
    RPC_PARSE_JSON = 0,
    RPC_PARSE_PROTOBUF = 1,
} rpc_parse_t;

typedef enum rpc_cmd {
    _RPC_TEST = 0,
    _RPC_GET_CONNECT_LIST = 1,
    _RPC_PEER_POST_MSG = 2,
    _RPC_SHELL_HELP = 3,
} rpc_cmd_t;

#define MAX_MESSAGES_IN_MAP         256
#define RPC_DIR_BIT                 24
#define RPC_CMD_BIT                 8
#define RPC_PARSE_BIT               0

#define RPC_TYPE_MASK               0xFFFF
#define RPC_CMD_MASK                0xF
#define RPC_DIR_MASK                0xF
#define RPC_PARSE_MASK              0xF

#define BUILD_RPC_TYPE(dir, cmd, parse) \
    (((((uint32_t)cmd) & RPC_CMD_MASK) << RPC_CMD_BIT) | \
     ((((uint32_t)dir) & RPC_DIR_MASK) << RPC_DIR_BIT) | \
     ((((uint32_t)parse) & RPC_PARSE_MASK) << RPC_PARSE_BIT))


#define RPC_TEST \
    BUILD_RPC_TYPE(RPC_DIR_UP, _RPC_TEST, RPC_PARSE_JSON)

#define RPC_GET_CONNECT_LIST \
    BUILD_RPC_TYPE(RPC_DIR_UP, _RPC_GET_CONNECT_LIST, RPC_PARSE_JSON)

#define RPC_PEER_POST_MSG \
    BUILD_RPC_TYPE(RPC_DIR_UP, _RPC_PEER_POST_MSG, RPC_PARSE_JSON)

#define RPC_SHELL_HELP \
    BUILD_RPC_TYPE(RPC_DIR_UP, _RPC_SHELL_HELP, RPC_PARSE_JSON)


#endif
