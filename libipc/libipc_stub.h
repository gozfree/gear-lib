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

#endif
