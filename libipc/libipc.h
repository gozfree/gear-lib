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

typedef struct ipc_header {
    uint32_t func_id;
    uint64_t time_stamp;
    uint32_t len;
} ipc_header_t;

typedef struct ipc_packet {
    struct ipc_header header;
    void *data;

} ipc_packet_t;

typedef struct ipc {
    void *ctx;
    int fd;
    struct ipc_packet packet;

} ipc_t;

typedef enum ipc_role {
    IPC_SENDER = 0,
    IPC_RECVER = 1,
} ipc_role;

struct ipc_ops {
    void *(*init)(enum ipc_role role);
    void (*deinit)(struct ipc *ipc);
    int (*send)(struct ipc *i, const void *buf, size_t len);
    int (*recv)(struct ipc *i, void *buf, size_t len);
    int (*unicast)();//TODO
    int (*broadcast)();//TODO
};


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
