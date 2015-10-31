/*****************************************************************************
 * Copyright (C) 2014-2015
 * file:    librpc.h
 * author:  gozfree <gozfree@163.com>
 * created: 2015-05-06 23:54
 * updated: 2015-05-06 23:54
 *****************************************************************************/
#ifndef _LIBRPC_H_
#define _LIBRPC_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "librpc_stub.h"

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************
 * rpc packet format:
 * [rpc_header][rpc_data]
 *
 * rpc_header format:
 * [uuid_dst][uuid_src][rpc_type][   len]
 * [ 20bytes][ 20bytes][  4bytes][4bytes]
 *
 * uuid format:
 * [    fd][remote_ip][remote_port]
 * [8bytes][   8bytes][     4bytes]
 * broadcast uuid
 * uuid_dst=FFFFFFFFFFFFFFFFFFFFFFFF
 *
 * rpc_type format:
 * [direct][command][parse format]
 * [ 1byte][  2byte][       1byte]
 *
 * direct format:
 * 00: c->s
 * 01: s->c
 * 02: c->s->c
 * 03: broadcast
 *
 * command format:
 *
 * parse format:
 * 01: json
 * 02: protocolbuf
 *********************************************
 */

#define MAX_UUID_LEN    (21)
#define UUID_BROADCAST  ((const char *)"FFFFFFFFFFFFFFFFFFFFFFFF")

/* rpc_header format: */
typedef struct rpc_header {
    char uuid_dst[MAX_UUID_LEN];
    char uuid_src[MAX_UUID_LEN];
    uint64_t time_stamp;
    uint32_t type;
    uint32_t len;
} rpc_header_t;

typedef struct rpc_packet {
    struct rpc_header header;
    void *data;
} rpc_packet_t;

typedef struct rpc {
    int fd;
    struct rpc_packet packet;
    struct gevent_base *evbase;
    struct gevent *ev;

} rpc_t;

typedef struct iobuf {
    void *addr;
    uint32_t len;
} iobuf_t;


typedef int (*rpc_callback)(struct rpc *r, void *arg, int len);

typedef struct msg_handler {
    uint32_t msg_id;
    rpc_callback cb;
} msg_handler_t;

struct rpc *rpc_create(const char *host, uint16_t port);
int rpc_set_cb(struct rpc *r,
        void (*on_read)(int fd, void *arg),
        void (*on_write)(int fd, void *arg),
        void (*on_error)(int fd, void *arg),  void *arg);
int rpc_echo(struct rpc *r, const void *buf, size_t len);
int rpc_send(struct rpc *r, const void *buf, size_t len);
int rpc_recv(struct rpc *r, void *buf, size_t len);
int rpc_call(struct rpc *r, int func_id,
            void *in_arg, size_t in_len,
            void *out_arg, size_t out_len);
struct iobuf *rpc_recv_buf(struct rpc *r);
int rpc_dispatch(struct rpc *r);
void rpc_destroy(struct rpc *r);
int rpc_header_format(struct rpc *r,
        char *uuid_dst, char *uuid_src, uint32_t type, size_t len);

int rpc_packet_format(struct rpc *r, uint32_t type);
int rpc_packet_parse(struct rpc *r);
int find_msg_handler(uint32_t msg_id, msg_handler_t *handler);
int process_msg2(struct rpc *r, struct iobuf *buf);
void dump_buffer(void *buf, int len);
void dump_packet(struct rpc_packet *r);


int register_msg_map(msg_handler_t *map, int num_entry);


#ifdef __cplusplus
}
#endif
#endif
