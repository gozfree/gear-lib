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
#include <libposix.h>
#include "libp2p.h"
#include "libstun.h"
#include "libptcp.h"
#include <libsock.h>
#include <librpc.h>
#include <librpc_stub.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

static uint16_t _rpc_port = 12345;
static char _local_ip[INET_ADDRSTRLEN];
static uint16_t _local_port = 0;
static struct p2p *_p2p = NULL;
int _p2p_connect(struct p2p *p2p, char *ip, uint16_t port);
void *tmp_thread(void *arg);

#define MAX_UUID_LEN                (21)

static int on_get_connect_list_resp(struct rpc_session *r, void *arg, size_t len, void **obuf, size_t *olen)
{
    char *ptr;
    int num = 0;
    printf("on_get_connect_list, len = %zu\n", len);
    num = len / MAX_UUID_LEN;
    for (ptr = (char *)arg; num > 0; --num) {
        printf("uuid list: %s\n", ptr);
        len = MAX_UUID_LEN;
        ptr += len;
    }
    return 0;
}

static int on_peer_post_msg_resp(struct rpc_session *r, void *arg, size_t len, void **obuf, size_t *olen)
{
    pthread_t tid;
    struct p2p *p2p = _p2p;
    uint32_t peer_id;
    char localip[SOCK_ADDR_LEN];
    char reflectip[SOCK_ADDR_LEN];
    struct sockaddr_in si;
    printf("on_peer_post_msg_resp len = %zu\n", len);
    struct nat_info *nat = (struct nat_info *)arg;
    printf("get nat info from peer\n");
    printf("nat.uuid = 0x%08x\n", nat->uuid);
    sock_addr_ntop(localip, nat->local.ip);
    sock_addr_ntop(reflectip, nat->reflect.ip);
    printf("nat.type = %d\n", nat->type);
    printf("nat.local_addr %s:%d\n", localip, nat->local.port);
    printf("nat.reflect_addr %s:%d\n", reflectip, nat->reflect.port);
    p2p->ptcp = ptcp_socket_by_fd(p2p->nat.fd);
    if (p2p->ptcp == NULL) {
        printf("error!\n");
        return -1;
    }
    if (p2p->rpc_state == P2P_RPC_SYN_SENT) {//client
        printf("as p2p client\n");
        sleep(1);
        if (_p2p_connect(p2p, localip, nat->local.port)) {
            printf("_p2p_connect nat.local failed, try nat.reflect\n");
            if (_p2p_connect(p2p, reflectip, nat->reflect.port)) {
                printf("_p2p_connect nat.reflect failed too\n");
                return -1;
            }
        }
        return 0;
    }
//server
    printf("as p2p server\n");
    peer_id = nat->uuid;
    p2p_connect(p2p, peer_id);

    si.sin_family = AF_INET;
    si.sin_addr.s_addr = inet_addr(_local_ip);
    si.sin_port = htons(_local_port);

    printf("ptcp_bind %s:%d\n", _local_ip, _local_port);
    ptcp_bind(p2p->ptcp, (struct sockaddr*)&si, sizeof(si));
    ptcp_listen(p2p->ptcp, 0);
    pthread_create(&tid, NULL, tmp_thread, p2p);
    return 0;
}

BEGIN_RPC_MAP(RPC_CLIENT_API)
RPC_MAP(RPC_GET_CONNECT_LIST, on_get_connect_list_resp)
RPC_MAP(RPC_PEER_POST_MSG, on_peer_post_msg_resp)
END_RPC_MAP()

static int rpc_get_connect_list(struct rpc *r)
{
    int len = 100;
    char *buf = (char *)calloc(1, len);
    memset(buf, 0xA5, len);
    rpc_call(r, RPC_GET_CONNECT_LIST, buf, len, NULL, 0);
    //printf("func_id = %x\n", RPC_GET_CONNECT_LIST);
    //dump_packet(&r->packet);
    return 0;
}

void p2p_get_peer_list(struct p2p *p2p)
{
    rpc_get_connect_list(p2p->rpc);
}

static int rpc_peer_post_msg(struct rpc *r, void *buf, size_t len)
{
    rpc_call(r, RPC_PEER_POST_MSG, buf, len, NULL, 0);
    //printf("func_id = %x\n", RPC_PEER_POST_MSG);
    //dump_packet(&r->packet);
    return 0;
}


int p2p_send(struct p2p *p2p, void *buf, int len)
{
    return ptcp_send(p2p->ptcp, buf, len, 0);
}

int p2p_recv(struct p2p *p2p, void *buf, int len)
{
    return ptcp_recv(p2p->ptcp, buf, len, 0);
}

int _p2p_connect(struct p2p *p2p, char *ip, uint16_t port)
{
    struct sockaddr_in si;
    si.sin_family = AF_INET;
    si.sin_addr.s_addr = inet_addr(ip);
    si.sin_port = htons(port);
    printf("ptcp_connect %s:%d\n", ip, port);
    if (0 != ptcp_connect(p2p->ptcp, (struct sockaddr*)&si, sizeof(si))) {
        printf("ptcp_connect timeout\n");
        return -1;
    } else {
        printf("ptcp_connect success\n");
    }
    return 0;
}

void *tmp_thread(void *arg)
{
    struct p2p *p2p = (struct p2p *)arg;
    int len;
    char buf[32] = {0};
    while (1) {
        memset(buf, 0, sizeof(buf));
        len = ptcp_recv(p2p->ptcp, buf, sizeof(buf), 0);
        if (len > 0) {
            printf("ptcp_recv len=%d, buf=%s\n", len, buf);
        } else if (len == 0) {
            printf("ptcp is closed\n");
        } else if (EWOULDBLOCK == errno){
            //printf("ptcp is error: %d\n", ptcp_get_error(p2p->ps));
            usleep(100 * 1000);
        }
    }
    return NULL;
}

#if 0
static void on_rpc_read(int fd, void *arg)
{
    struct p2p *p2p = (struct p2p *)arg;
    struct rpc *r = p2p->rpc;
    struct iovec *buf = rpc_recv_buf(r);
    if (!buf) {
        printf("rpc_recv_buf failed!\n");
        return;
    }
    process_msg(r, buf);
}

void on_rpc_read(int fd, void *arg)
{
    pthread_t tid;
    struct p2p *p2p = (struct p2p *)arg;
    struct rpc *r = p2p->rpc;
    char *peer_id;
    char localip[MAX_ADDR_STRING];
    char reflectip[MAX_ADDR_STRING];
    struct sockaddr_in si;
    struct iobuf *buf = rpc_recv_buf(r);
    if (!buf) {
        printf("rpc_recv_buf failed!\n");
        return;
    }
    struct nat_info *nat = (struct nat_info *)buf->addr;
    printf("peer info\n");
    printf("nat.uuid = %s\n", nat->uuid);
    sock_addr_ntop(localip, nat->local.ip);
    sock_addr_ntop(reflectip, nat->reflect.ip);
    printf("nat.type = %d, local.ip = %s, port = %d\n",
            nat->type, localip, nat->local.port);
    printf("reflect.ip = %s, port = %d\n",
            reflectip, nat->reflect.port);
    p2p->ps = ptcp_socket_by_fd(p2p->nat.fd);
    if (p2p->ps == NULL) {
        printf("error!\n");
        return;
    }
    if (p2p->rpc_state == P2P_RPC_SYN_SENT) {//client
        sleep(1);
        _p2p_connect(p2p, reflectip, nat->reflect.port);
        return;
    }
//server
    peer_id = nat->uuid;
    p2p_connect(p2p, peer_id);

    si.sin_family = AF_INET;
    si.sin_addr.s_addr = inet_addr(_local_ip);
    si.sin_port = htons(_local_port);

    printf("ptcp_bind %s:%d\n", _local_ip, _local_port);
    ptcp_bind(p2p->ps, (struct sockaddr*)&si, sizeof(si));
    ptcp_listen(p2p->ps, 0);
    pthread_create(&tid, NULL, tmp_thread, p2p);
}

static void on_rpc_write(int fd, void *arg)
{
    //printf("on_write fd= %d\n", fd);
}

static void on_rpc_error(int fd, void *arg)
{
    printf("on_error fd= %d\n", fd);
}
#endif

static int __rand(void)
{
    uint64_t tick;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    tick = (tv.tv_sec * 1000) + (tv.tv_usec / 1000);

    int seed = (int)tick;
    srandom(seed);
    return random();
}

static uint16_t random_port(void)
{
    uint16_t min = 0x4000;
    uint16_t max = 0x7FFF;

    int ret = __rand();
    ret = ret | min;
    ret = ret & max;

    return (uint16_t)ret;
}

struct p2p *p2p_init(const char *rpc_srv, const char *stun_srv)
{
    char ip[64];
    struct sock_addr tmpaddr;
    static stun_addr _mapped;
    struct p2p *p2p = CALLOC(1, struct p2p);
    if (!p2p) {
        printf("malloc failed: %d\n", errno);
        return NULL;
    }

    p2p->rpc = rpc_client_create(rpc_srv, _rpc_port);
    if (!p2p->rpc) {
        printf("rpc_create failed\n");
        return NULL;
    }
    RPC_REGISTER_MSG_MAP(RPC_CLIENT_API);
    printf("RPC_TEST:%d\n", RPC_TEST);
    printf("RPC_GET_CONNECT_LIST:%d\n", RPC_GET_CONNECT_LIST);
    printf("RPC_PEER_POST_MSG:%d\n", RPC_PEER_POST_MSG);
    printf("RPC_SHELL_HELP:%d\n", RPC_SHELL_HELP);
    printf("RPC_GET_CONNECT_LIST:%d\n", RPC_GET_CONNECT_LIST);

    sock_getaddr_by_fd(p2p->rpc->base.fd, &tmpaddr);
    sock_addr_ntop(_local_ip, tmpaddr.ip);
    //_local_port = tmpaddr.port;
    //printf("_local_port = %d\n", _local_port);

    stun_init(&p2p->stun, stun_srv);
    p2p->nat.type = stun_nat_type(&p2p->stun);
    printf("%s:%d xxxx\n", __func__, __LINE__);
    p2p->nat.uuid = p2p->rpc->uuid;
    p2p->nat.local.ip = sock_addr_pton(_local_ip);

    _local_port = random_port();
    p2p->nat.local.port = _local_port;
    p2p->nat.fd = stun_socket(&p2p->stun, _local_ip, _local_port, &_mapped);
    _mapped.addr = ntohl(_mapped.addr);
    sock_addr_ntop(ip, _mapped.addr);
    p2p->nat.reflect.ip = _mapped.addr;
    p2p->nat.reflect.port = _mapped.port;
    printf("get nat info from local\n");
    printf("nat.type = %d\n", p2p->nat.type);
    printf("nat.local_addr %s:%d\n", _local_ip, p2p->nat.local.port);
    printf("nat.reflect_addr %s:%d\n", ip, p2p->nat.reflect.port);
    p2p->rpc_state = P2P_RPC_INIT;
    _p2p = p2p;
    return p2p;
}

int p2p_connect(struct p2p *p2p, uint32_t peer_id)
{
    int len = (int)sizeof(struct nat_info);
    printf("rpc_peer_post_msg from %x to %x\n", p2p->rpc->uuid, peer_id);
    p2p->rpc->uuid = peer_id;
    //rpc_send(p2p->rpc, (void *)&p2p->nat, len);
    printf("rpc_peer_post_msg len = %d\n", len);
    rpc_peer_post_msg(p2p->rpc, (void *)&p2p->nat, len);
    p2p->rpc_state = P2P_RPC_SYN_SENT;

    return 0;
}

int p2p_dispatch(struct p2p *p2p)
{
    return rpc_client_dispatch(p2p->rpc);
}

void p2p_deinit(struct p2p *p)
{

}
