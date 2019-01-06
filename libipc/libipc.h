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
#ifndef LIBIPC_H
#define LIBIPC_H

#include <libdict.h>
#include <libgevent.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>
#include <semaphore.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_IPC_RESP_BUF_LEN        (1024)
#define MAX_IPC_MESSAGE_SIZE        (1024)
#define MAX_MESSAGES_IN_MAP         (256)

#define IPC_SERVER_PORT             (5555)
typedef enum ipc_role {
    IPC_SERVER = 0,
    IPC_CLIENT = 1,
} ipc_role;

struct ipc;

typedef int (*ipc_callback)(struct ipc *ipc,
                void *in_arg, size_t in_len,
                void *out_arg, size_t *out_len);

typedef struct ipc_handler {
    uint32_t func_id;
    ipc_callback cb;
} ipc_handler_t;

typedef struct ipc_header {
    uint32_t func_id;
    uint64_t time_stamp;
    uint32_t payload_len;
} ipc_header_t;

typedef struct ipc_packet {
    struct ipc_header header;
    uint8_t payload[0];
} ipc_packet_t;

typedef int (ipc_recv_cb)(struct ipc *ipc, void *buf, size_t len);
struct ipc_ops {
    void *(*init)(struct ipc *ipc, uint16_t port, enum ipc_role role);
    void (*deinit)(struct ipc *ipc);
    int (*accept)(struct ipc *ipc);
    int (*connect)(struct ipc *ipc, const char *name);
    int (*register_recv_cb)(struct ipc *i, ipc_recv_cb cb);
    int (*send)(struct ipc *i, const void *buf, size_t len);
    int (*recv)(struct ipc *i, void *buf, size_t len);
    int (*unicast)();//TODO
    int (*broadcast)();//TODO
};

typedef struct ipc {
    void *ctx;
    int fd;
    int afd;
    enum ipc_role role;
    struct ipc_packet packet;
    const struct ipc_ops *ops;
    sem_t sem;
    void *resp_buf;//async response buffer;
    int resp_len;
    dict *async_cmd_list;
    pthread_t tid;
    struct gevent_base *evbase;
} ipc_t;

struct ipc *ipc_create(enum ipc_role role, uint16_t port);
void ipc_destroy(struct ipc *ipc);

int ipc_call(struct ipc *i, uint32_t func_id,
             const void *in_arg, size_t in_len,
             void *out_arg, size_t out_len);

void ipc_destroy(struct ipc *i);

int ipc_register_map(ipc_handler_t *map, int num_entry);

#define IPC_REGISTER_MAP(map_name)             \
    ipc_register_map(__ipc_action_map##map_name,  \
                    (sizeof(__ipc_action_map##map_name )/sizeof(ipc_handler_t)));

#define BEGIN_IPC_MAP(map_name)  \
    static ipc_handler_t  __ipc_action_map##map_name[] = {
#define IPC_MAP(x, y) {x, y},
#define END_IPC_MAP() };


/******************************************************************************
 * ipc_packet define (little endian)
 * [ipc_header][ipc_payload]
 *
 * ipc_header define
 * +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 * |7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0|
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                           message_id=32                       |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                                                               |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+ time_stamp=64 +-+-+-+-+-+-+-+-+-+-+-+
 * |                                                               |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                           payload_len=32                      |
 * +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * message_id define
 * +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 * |7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0|
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  group_id=7 |unused=5 |R|D|P=2|         cmd_id=16             |
 * +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *  [31~25]: group id
 *         - max support 128 group, can be used to service group
 *  [24~20]: unused
 *  [   19]: return indicator
 *         - 0: no need return
 *         - 1: need return
 *  [   18]: direction
 *         - 0: UP client to server
 *         - 1: DOWN server to client
 *  [17~16]: parser of payload message
 *         - 0 json
 *         - 1 protobuf
 *         - 2 unused
 *         - 3 unused
 *  [15~ 0]: cmd id, defined in libipc_stub.h
 *         - 0 ~ 7 inner cmd
 *         - 8 ~ 255 user cmd
 *
 * Note: how to add a new bit define, e.g. foo:
 *       1. add foo define in this commet;
 *       2. define IPC_FOO_BIT and IPC_FOO_MASK, and add into BUILD_IPC_MSG_ID;
 *       3. define GET_IPC_FOO;
 *       4. define enum of foo value;
 ******************************************************************************/

#define IPC_MSG_ID_MASK             0xFFFFFFFF


#define IPC_GROUP_BIT               (25)
#define IPC_GROUP_MASK              0x07

#define IPC_MAGIC_BIT               (20)
#define IPC_MAGIC_MASK              0x1F
#define IPC_MAGIC_ID                (0x1B) //11011b

#define IPC_RET_BIT                 (19)
#define IPC_RET_MASK                0x01

#define IPC_DIR_BIT                 (18)
#define IPC_DIR_MASK                0x01

#define IPC_PARSE_BIT               (16)
#define IPC_PARSE_MASK              0x03

#define IPC_CMD_BIT                 (0)
#define IPC_CMD_MASK                0xFF


#define BUILD_IPC_MSG_ID(group, ret, dir, parse, cmd) \
    (((((uint32_t)group) & IPC_GROUP_MASK) << IPC_GROUP_BIT) | \
     ((((uint32_t)IPC_MAGIC_ID) & IPC_MAGIC_MASK) << IPC_MAGIC_BIT) | \
     ((((uint32_t)ret) & IPC_RET_MASK) << IPC_RET_BIT) | \
     ((((uint32_t)dir) & IPC_DIR_MASK) << IPC_DIR_BIT) | \
     ((((uint32_t)parse) & IPC_PARSE_MASK) << IPC_PARSE_BIT) | \
     ((((uint32_t)cmd) & IPC_CMD_MASK) << IPC_CMD_BIT))

#define IS_IPC_MSG_NEED_RETURN(cmd) \
        (((cmd & IPC_MSG_ID_MASK)>>IPC_RET_BIT) & IPC_RET_MASK)

#define GET_IPC_MSG_GROUP(cmd) \
        (((cmd & IPC_MSG_ID_MASK)>>IPC_GROUP_BIT) & IPC_GROUP_MASK)

#define GET_IPC_MSG_DIR(cmd) \
        (((cmd & IPC_MSG_ID_MASK)>>IPC_DIR_BIT) & IPC_DIR_MASK)

#define GET_IPC_MSG_PARSE(cmd) \
        (((cmd & IPC_MSG_ID_MASK)>>IPC_PARSE_BIT) & IPC_PARSE_MASK)

#define IS_IPC_MSG_VALID(cmd) \
        ((((cmd & IPC_MSG_ID_MASK)>>IPC_MAGIC_BIT) & IPC_MAGIC_MASK)==IPC_MAGIC_ID)


enum ipc_direction {
    _IPC_DIR_UP = 0,
    _IPC_DIR_DOWN = 1,
};

enum ipc_parser {
    _IPC_PARSE_JSON = 0,
    _IPC_PARSE_PROTOBUF = 1,
};

enum ipc_return {
    _IPC_NO_RETURN = 0,
    _IPC_NEED_RETURN = 1,
};

enum ipc_cmd_inner {
    _IPC_INNER_0    = 0,
    _IPC_INNER_1    = 1,
    _IPC_INNER_2    = 2,
    _IPC_INNER_3    = 3,
    _IPC_INNER_4    = 4,
    _IPC_INNER_5    = 5,
    _IPC_INNER_6    = 6,
    _IPC_INNER_7    = 7,
    _IPC_USER_BASE  = 8,
};


#ifdef __cplusplus
}
#endif
#endif
