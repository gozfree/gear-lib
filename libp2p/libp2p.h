/*****************************************************************************
 * Copyright (C) 2014-2015
 * file:    libp2p.h
 * author:  gozfree <gozfree@163.com>
 * created: 2015-05-19 23:57
 * updated: 2015-05-19 23:57
 *****************************************************************************/
#ifndef LIBP2P_H
#define LIBP2P_H

#include <libskt.h>
#include <librpc.h>
#include <libptcp.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct nat_info {
    int fd;
    int type;
    struct skt_addr local;
    struct skt_addr reflect;
    uint32_t uuid;
} nat_info_t;

typedef enum p2p_rpc_state {
    P2P_RPC_INIT,
    P2P_RPC_SYN_SENT,
} p2p_rpc_state_t;


typedef struct p2p {
    struct rpc *rpc;
    struct nat_info nat;
    ptcp_socket_t *ps;
    enum p2p_rpc_state rpc_state;
} p2p_t;

struct p2p *p2p_init(const char *rpc_srv, const char *stun_srv);
void p2p_get_peer_list(struct p2p *p2p);
int p2p_connect(struct p2p *p2p, uint32_t peer_id);
int p2p_dispatch(struct p2p *p2p);
int p2p_send(struct p2p *p2p, void *buf, int len);
int p2p_recv(struct p2p *p2p, void *buf, int len);
void p2p_deinit(struct p2p *p);


#ifdef __cplusplus
}
#endif
#endif
