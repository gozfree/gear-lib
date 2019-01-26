/******************************************************************************
 * Copyright (C) 2014-2020 Zhifeng Gong <gozfree@163.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
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
