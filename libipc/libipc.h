/******************************************************************************
 * Copyright (C) 2014-2015
 * file:    libipc.h
 * author:  gozfree <gozfree@163.com>
 * created: 2015-11-10 16:29:41
 * updated: 2015-11-10 16:29:41
 *****************************************************************************/
#ifndef _LIBIPC_H_
#define _LIBIPC_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_IPC_MESSAGE_SIZE     1024
#define MAX_MESSAGES_IN_MAP         256


typedef enum ipc_role {
    IPC_SENDER = 0,
    IPC_RECVER = 1,
} ipc_role;

struct ipc;
typedef int (*ipc_callback)(struct ipc *ipc,
                void *in_arg, size_t in_len,
                void *out_arg, size_t out_len);

typedef struct ipc_handler {
    uint32_t func_id;
    ipc_callback cb;
} ipc_handler_t;


typedef struct ipc_header {
    uint32_t func_id;
    uint64_t time_stamp;
    uint32_t len;
    uint32_t payload_len;
} ipc_header_t;

typedef struct ipc_packet {
    struct ipc_header header;
    void *data;

} ipc_packet_t;

typedef void (ipc_recv_cb)(struct ipc *ipc, void *buf, size_t len);
struct ipc_ops {
    void *(*init)(enum ipc_role role);
    void (*deinit)(struct ipc *ipc);
    int (*register_recv_cb)(struct ipc *i, ipc_recv_cb cb);
    int (*send)(struct ipc *i, const void *buf, size_t len);
    int (*recv)(struct ipc *i, void *buf, size_t len);
    int (*unicast)();//TODO
    int (*broadcast)();//TODO
};


typedef struct ipc {
    void *ctx;
    int fd;
    struct ipc_packet packet;
    const struct ipc_ops *ops;

} ipc_t;

struct ipc *ipc_create();

ssize_t ipc_send(struct ipc *i, const void *buf, size_t len);
ssize_t ipc_recv(struct ipc *i, void *buf, size_t len);

int ipc_call(struct ipc *i, uint32_t func_id,
             const void *in_arg, size_t in_len,
             void *out_arg, size_t out_len);

void ipc_destroy(struct ipc *i);

#ifdef __cplusplus
}
#endif
#endif
