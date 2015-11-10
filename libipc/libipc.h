/******************************************************************************
 * Copyright (C) 2014-2015
 * file:    libipc.h
 * author:  gozfree <gozfree@163.com>
 * created: 2015-11-10 16:29:41
 * updated: 2015-11-10 16:29:41
 *****************************************************************************/
#ifndef _LIBIPC_H_
#define _LIBIPC_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ipc_header {

} ipc_header_t;

typedef struct ipc_packet {
    struct ipc_header header;
    void *data;

} ipc_packet_t;

struct ipc {
    int fd;
    struct ipc_packet packet;

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
