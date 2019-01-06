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
#ifndef LIBP2P_H
#define LIBP2P_H

#include <libskt.h>
#include <librpc.h>
#include "libptcp.h"

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
