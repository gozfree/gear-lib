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
#include "librpc.h"
#include <gear-lib/libmacro.h>
#include <gear-lib/libgevent.h>
#include <gear-lib/libhash.h>
#include <gear-lib/libskt.h>
#include <gear-lib/libthread.h>
#include <gear-lib/libworkq.h>
#include <gear-lib/libtime.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <sys/time.h>
#include <sys/uio.h>

extern struct rpc_ops msgq_posix_ops;
extern struct rpc_ops msgq_sysv_ops;
extern struct rpc_ops socket_ops;
extern struct rpc_ops netlink_ops;
extern struct rpc_ops sharemem_ops;

struct rpc_backend {
    int id;
    struct rpc_ops *ops;
};

typedef enum rpc_backend_type {
    RPC_MQ_POSIX,
    RPC_MQ_SYSV,
    RPC_SOCKET,
    RPC_NETLINK,
    RPC_SHAREMEM,
} rpc_backend_type;

struct rpc_backend rpc_backend_list[] = {
    {RPC_MQ_POSIX, &msgq_posix_ops},
    {RPC_MQ_SYSV,  &msgq_sysv_ops},
    {RPC_SOCKET,   &socket_ops},
    {RPC_NETLINK,  &netlink_ops},
    {RPC_SHAREMEM, &sharemem_ops},
};

#define RPC_BACKEND RPC_SOCKET
#define MAX_UUID_LEN                (21)

struct wq_arg {
    msg_handler_t handler;
    struct rpc r;
    void *buf;
    size_t len;
};

static struct hash *_msg_map_registered = NULL;

void dump_buffer(void *buf, int len)
{
    int i;
    for (i = 0; i < len; i++) {
        if (!(i%16))
           printf("\n%p: ", (char *)buf+i);
        printf("%02x ", (*((char *)buf + i)) & 0xff);
    }
    printf("\n");
}

void dump_packet(struct rpc_packet *r)
{
    printf("packet header:\n");
    dump_buffer(&r->header, (int)sizeof(struct rpc_header));
    printf("packet payload:\n");
    dump_buffer(r->payload, r->header.payload_len);
}

void print_packet(struct rpc_packet *pkt)
{
    char ts[64];
    printf("================\n");
    printf("rpc_packet[%p]:\n", pkt);
    printf("header.uuid_dst    = 0x%08x\n", pkt->header.uuid_dst);
    printf("header.uuid_src    = 0x%08x\n", pkt->header.uuid_src);
    printf("header.msg_id      = 0x%08x\n", pkt->header.msg_id);
    printf("header.timestamp   = %" PRIu64 "\n", pkt->header.timestamp);
    printf("header.timestamp   = %s\n",
            time_str_human_by_msec(pkt->header.timestamp, ts, sizeof(ts)));
    printf("header.payload_len = %d\n", pkt->header.payload_len);
    printf("header.checksum    = 0x%08x\n", pkt->header.checksum);
    dump_packet(pkt);
    printf("================\n");
}

void process_wq(void *arg)
{
    struct wq_arg *wq = (struct wq_arg *)arg;
    if (&wq->handler) {
        wq->handler.cb(&wq->r, wq->buf, wq->len);
    }
}

static size_t pack_msg(struct rpc_packet *pkt, uint32_t msg_id,
                const void *in_arg, size_t in_len)
{
    struct rpc_header *hdr;
    size_t pkt_len;
    struct time_info ti;
    if (!pkt) {
        printf("invalid paraments!\n");
        return 0;
    }

    hdr = &(pkt->header);
    hdr->msg_id = msg_id;
    time_info(&ti);
    hdr->timestamp = ti.utc_msec;

    if (in_arg) {
        hdr->payload_len = in_len;
        pkt->payload = (void *)in_arg;
    } else {
        hdr->payload_len = 0;
        pkt->payload = NULL;
    }
    pkt_len = sizeof(struct rpc_packet) + hdr->payload_len;
    return pkt_len;
}

static size_t unpack_msg(struct rpc_packet *pkt, uint32_t *msg_id,
                void *out_arg, size_t *out_len)
{
    size_t pkt_len;
    struct rpc_header *hdr;
    if (!pkt || !out_arg || !out_len) {
        printf("invalid paraments!\n");
        return -1;
    }
    hdr = &(pkt->header);
    *msg_id = hdr->msg_id;
    *out_len = hdr->payload_len;
    memcpy(out_arg, pkt->payload, *out_len);
    pkt_len = sizeof(struct rpc_packet) + hdr->payload_len;
    return pkt_len;
}

int rpc_send(struct rpc *r, const void *buf, size_t len)
{
    int ret, head_size;
    struct rpc_packet *pkt = &r->send_pkt;
    struct time_info ti;

    time_info(&ti);
    pkt->header.timestamp = ti.utc_msec;
    pkt->header.payload_len = len;
    pkt->payload = (void *)buf;
    head_size = sizeof(rpc_header_t);

    ret = r->ops->send(r, (void *)&pkt->header, head_size);
    if (ret != head_size) {
        printf("send failed!\n");
        return -1;
    }
    //printf("rpc_send:\n");
    //print_packet(&r->send_pkt);
    ret = r->ops->send(r, pkt->payload, pkt->header.payload_len);
    if (ret != (int)pkt->header.payload_len) {
        printf("send failed!\n");
        return -1;
    }
    return (head_size + ret);
}

static int do_process_msg(struct rpc *r, void *buf, size_t len)
{
    char uuid_str[4];
    int ret;
    msg_handler_t *msg_handler;
    struct rpc_header *h = &r->recv_pkt.header;
    int msg_id = rpc_packet_parse(r);
    printf("msg_id = %08x\n", msg_id);

    msg_handler = find_msg_handler(msg_id);
    if (msg_handler) {
        struct wq_arg *arg = CALLOC(1, struct wq_arg);
        memcpy(&arg->handler, msg_handler, sizeof(msg_handler_t));
        memcpy(&arg->r, r, sizeof(struct rpc));
        arg->buf = calloc(1, len);
        memcpy(arg->buf, buf, len);
        arg->len = len;
        wq_task_add(r->wq, process_wq, arg, sizeof(struct wq_arg));
    } else {
        printf("no callback for this MSG ID(%d) in process_msg\n", msg_id);
        snprintf(uuid_str, sizeof(uuid_str), "%x", h->uuid_dst);
        char *valfd = (char *)hash_get(r->dict_uuid2fd, uuid_str);
        if (!valfd) {
            printf("hash_get failed: key=%x\n", h->uuid_dst);
            return -1;
        }
        int dst_fd = strtol(valfd, NULL, 16);
        r->fd = dst_fd;
        printf("%s:%d before rpc_send\n", __func__, __LINE__);
        ret = rpc_send(r, buf, len);
    }
    return ret;
}


struct iovec *rpc_recv_buf(struct rpc *r)
{
    struct iovec *buf = CALLOC(1, struct iovec);
    struct rpc_packet *recv_pkt = &r->recv_pkt;
    uint32_t uuid_dst;
    uint32_t uuid_src;
    int ret;
    int head_size = sizeof(rpc_header_t);

    ret = r->ops->recv(r, (void *)&recv_pkt->header, head_size);
    if (ret == 0) {
        //printf("peer connect closed\n");
        goto err;
    } else if (ret != head_size) {
        printf("recv failed, head_size = %d, ret = %d\n", head_size, ret);
        goto err;
    }
    uuid_src = r->recv_pkt.header.uuid_src;
    uuid_dst = r->send_pkt.header.uuid_src;
    if (uuid_src != 0 && uuid_dst != 0 && uuid_src != uuid_dst) {
        printf("uuid_src(0x%08x) is diff from received uuid_dst(0x%08x)\n", uuid_src, uuid_dst);
        printf("this maybe a peer call\n");
    } else {
        //printf("uuid match.\n");
    }
    buf->iov_len = recv_pkt->header.payload_len;
    buf->iov_base = calloc(1, buf->iov_len);
    recv_pkt->payload = buf->iov_base;
    ret = r->ops->recv(r, buf->iov_base, buf->iov_len);
    if (ret == 0) {
        printf("peer connect closed\n");
        goto err;
    } else if (ret != (int)buf->iov_len) {
        printf("recv failed: rlen=%zu, ret=%d\n", buf->iov_len, ret);
        goto err;
    }
    //printf("rpc_recv:\n");
    //print_packet(&r->recv_pkt);
    return buf;
err:
    if (buf->iov_base) {
        free(buf->iov_base);
    }
    free(buf);
    return NULL;
}

int rpc_recv(struct rpc *r, void *buf, size_t len)
{
    int ret, rlen;
    struct rpc_packet *pkt = &r->recv_pkt;
    int head_size = sizeof(rpc_header_t);
    memset(pkt, 0, sizeof(rpc_packet_t));
    pkt->payload = buf;

    ret = r->ops->recv(r, (void *)&pkt->header, head_size);
    if (ret == 0) {
        printf("peer connect closed\n");
        return -1;
    } else if (ret != head_size) {
        printf("recv failed, head_size = %d, ret = %d\n", head_size, ret);
        return -1;
    }
    if (r->send_pkt.header.uuid_dst != pkt->header.uuid_dst) {
        printf("uuid_dst is diff from recved packet.header.uuid_dst!\n");
    }
    if (len < pkt->header.payload_len) {
        printf("recv pkt.header.len = %d\n", pkt->header.payload_len);
    }
    rlen = MIN2(len, pkt->header.payload_len);
    ret = r->ops->recv(r, buf, rlen);
    if (ret == 0) {
        printf("peer connect closed\n");
        return -1;
    } else if (ret != rlen) {
        printf("recv failed: rlen=%d, ret=%d\n", rlen, ret);
        return -1;
    }
    //printf("rpc_recv:\n");
    //print_packet(&r->recv_pkt);
    return ret;
}

msg_handler_t *find_msg_handler(uint32_t msg_id)
{
    char msg_id_str[11];
    msg_handler_t *handler;
    snprintf(msg_id_str, sizeof(msg_id_str), "0x%08x", msg_id);
    msg_id_str[10] = '\0';
    handler = (msg_handler_t *)hash_get(_msg_map_registered, msg_id_str);
    return handler;

}

int process_msg(struct rpc *r, void *buf, size_t len)
{
    msg_handler_t *msg_handler;

    int msg_id = rpc_packet_parse(r);
    msg_handler = find_msg_handler(msg_id);
    if (msg_handler) {
        msg_handler->cb(r, buf, len);
    } else {
        //printf("no callback for this MSG ID(0x%08x) in process_msg\n", msg_id);
    }
    return 0;
}

static void on_client_init(int fd, void *arg)
{
    struct rpc *r = (struct rpc *)arg;
    char out_arg[1024];
    struct iovec *recv_buf;
    uint32_t msg_id;
    size_t out_len;
    memset(&r->recv_pkt, 0, sizeof(struct rpc_packet));

    recv_buf = rpc_recv_buf(r);
    if (!recv_buf) {
        printf("rpc_recv_buf failed!\n");
        return;
    }

    struct rpc_packet *pkt = &r->recv_pkt;
    unpack_msg(pkt, &msg_id, &out_arg, &out_len);

    printf("distribute msg_id = %08x\n", msg_id);
    if (r->state == rpc_inited) {
        r->send_pkt.header.uuid_src = *(uint32_t *)pkt->payload;
        thread_lock(r->dispatch_thread);
        thread_signal(r->dispatch_thread);
        thread_unlock(r->dispatch_thread);
        r->state = rpc_connected;
    } else if (r->state == rpc_connected) {
        struct iovec buf;
        buf.iov_len = pkt->header.payload_len;
        buf.iov_base = pkt->payload;
        process_msg(r, buf.iov_base, buf.iov_len);
        //free(pkt->payload);
        thread_lock(r->dispatch_thread);
        thread_signal(r->dispatch_thread);
        thread_unlock(r->dispatch_thread);
    }
}

#if 0
static void on_write(int fd, void *arg)
{
//    printf("on_write fd= %d\n", fd);
}

static void on_error(int fd, void *arg)
{
//    printf("on_error fd= %d\n", fd);
}

int rpc_dispatch(struct rpc *r)
{
    gevent_base_loop(r->evbase);
    return 0;
}

static void *rpc_dispatch_thread(struct thread *t, void *arg)
{
    struct rpc *r = (struct rpc *)arg;
    rpc_dispatch(r);
    return NULL;
}
#endif

static int push_async_cmd(struct rpc *rpc, uint32_t func_id)
{
    char cmd_str[11];//0xFFFFFFFF
    snprintf(cmd_str, sizeof(cmd_str), "0x%08X", func_id);
    if (-1 == hash_set(rpc->dict_async_cmd, cmd_str, cmd_str)) {
        return -1;
    }
    return 0;
}

static int pop_async_cmd(struct rpc *rpc, uint32_t func_id)
{
    int ret = 0;
    char *pair = NULL;
    char cmd_str[11];//0xFFFFFFFF
    snprintf(cmd_str, sizeof(cmd_str), "0x%08X", func_id);
    pair = hash_get(rpc->dict_async_cmd, cmd_str);
    if (pair) {
        if (-1 == hash_del(rpc->dict_async_cmd, cmd_str)) {
            ret = -1;
        } else {
            ret = 0;
        }
    } else {
        ret = -1;
    }
    return ret;
}


static int on_return(struct rpc *rpc, void *buf, size_t len)
{
    uint32_t func_id;
    size_t out_len;
    struct rpc_packet *pkt = (struct rpc_packet *)buf;
    memset(rpc->resp_buf, 0, MAX_RPC_RESP_BUF_LEN);
    if (-1 == unpack_msg(pkt, &func_id, rpc->resp_buf, &out_len)) {
        printf("unpack_msg failed!\n");
        return -1;
    }
    if (-1 == pop_async_cmd(rpc, func_id)) {
        printf("msg received is not the response of cmd_id!\n");
        return -1;
    }

    rpc->resp_len = out_len;
    sem_post(&rpc->sem);
    printf("%s:%d xxxx\n", __func__, __LINE__);
    return 0;
}

int rpc_set_cb(struct rpc *r,
        void (*cbr)(int fd, void *arg),
        void (*cbw)(int fd, void *arg),
        void (*cbe)(int fd, void *arg),  void *arg)
{
    if (!r) {
        printf("invalid parament!\n");
        return -1;
    }
    struct gevent *e = gevent_create(r->fd, cbr, cbw, cbe, arg);
    if (!e) {
        printf("gevent_create failed!\n");
    }
    if (-1 == gevent_add(r->evbase, e)) {
        printf("event_add failed!\n");
    }
    return 0;
}

int rpc_packet_parse(struct rpc *r)
{
    struct rpc_header *h = &r->recv_pkt.header;
    return h->msg_id;
}

static int register_msg_proc(msg_handler_t *handler)
{
    uint32_t msg_id;
    char msg_id_str[11];
    char *msg_proc;

    if (!handler) {
        printf("Cannot register null msg proc\n");
        return -1;
    }
    msg_id = handler->msg_id;
    snprintf(msg_id_str, sizeof(msg_id_str), "0x%08x", msg_id);
    msg_id_str[10] = '\0';
    if (!_msg_map_registered) {
            _msg_map_registered = hash_create(10240);
            if (!_msg_map_registered) {
                    printf("create message map table failed!\n");
                    return -1;
            }
    }
    msg_proc = hash_get(_msg_map_registered, msg_id_str);
    if (msg_proc) {//force update
        if (0 != hash_del(_msg_map_registered, msg_id_str)) {
            printf("hash_del failed!\n");
        }
    }
    if (0 != hash_set(_msg_map_registered, msg_id_str, (char *)handler)) {
        printf("hash_add failed!\n");
    }
    return 0;
}

int register_msg_map(msg_handler_t *map, int num_entry)
{
    int i;
    int ret;

    if (map == NULL) {
        printf("register_msg_map: null map \n");
        return -1;
    }

    if (num_entry <= 0) {
        printf("register_msg_map:invalid num_entry %d \n", num_entry);
        return -1;
    }

    for (i = 0; i < num_entry; i++) {
        ret = register_msg_proc(&map[i]);
        if (ret < 0) {
            printf("register_msg_map:register failed  at %d \n", i);
            return -1;
        }
    }

    return 0;
}

int rpc_call(struct rpc *r, uint32_t msg_id,
                const void *in_arg, size_t in_len,
                void *out_arg, size_t out_len)
{
    if (!r) {
        printf("invalid parament!\n");
        return -1;
    }
    size_t pkt_len = pack_msg(&r->send_pkt, msg_id, in_arg, in_len);
    if (pkt_len == 0) {
        printf("pack_msg failed!\n");
        return -1;
    }
    printf("%s:%d before rpc_send\n", __func__, __LINE__);
    if (0 > rpc_send(r, in_arg, in_len)) {
        printf("rpc_send failed, fd = %d!\n", r->fd);
        return -1;
    }
    if (IS_RPC_MSG_NEED_RETURN(msg_id)) {
        if (-1 == push_async_cmd(r, msg_id)) {
            printf("push_async_cmd failed!\n");
        }
        thread_lock(r->dispatch_thread);
        if (thread_wait(r->dispatch_thread, 2000) == -1) {
            printf("wait response failed %d:%s\n", errno, strerror(errno));
            return -1;
        }
        thread_unlock(r->dispatch_thread);
        memcpy(out_arg, r->recv_pkt.payload, out_len);
    } else {

    }
    return 0;
}

int rpc_peer_call(struct rpc *r, uint32_t uuid, uint32_t cmd_id,
            const void *in_arg, size_t in_len,
            void *out_arg, size_t out_len)
{
    r->send_pkt.header.uuid_dst = uuid;
    return rpc_call(r, cmd_id, in_arg, in_len, out_arg, out_len);
}

struct rpc *rpc_client_create(const char *host, uint16_t port)
{
    struct rpc *r = CALLOC(1, struct rpc);
    if (!r) {
        printf("malloc failed!\n");
        return NULL;
    }
    memset(&r->recv_pkt, 0, sizeof(struct rpc_packet));
    r->role = RPC_CLIENT;
    r->ops = rpc_backend_list[RPC_BACKEND].ops;
    r->on_client_init = on_client_init;

    r->dict_async_cmd = hash_create(1024);
    if (!r->dict_async_cmd) {
        printf("hash_create dict_async_cmd failed!\n");
    }
    r->resp_buf = calloc(1, MAX_RPC_MESSAGE_SIZE);
    r->ops->register_recv_cb(r, on_return);
    sem_init(&r->sem, 0, 0);
    r->ctx = r->ops->init(r, host, port, r->role);
    if (!r->ctx) {
        printf("init failed!\n");
    }
    r->state = rpc_inited;
    printf("rpc_client_create success\n");
    return r;
}

int rpc_connect_add(struct rpc *rpc, struct rpc *r, int fd, uint32_t uuid)
{
    char fd_str[9];
    char uuid_str[9];
    char *fdval = (char *)calloc(1, 9);
    snprintf(fd_str, sizeof(fd_str), "%08x", fd);
    snprintf(uuid_str, sizeof(uuid_str), "%08x", uuid);
    snprintf(fdval, 9, "%08x", fd);
    hash_set(rpc->dict_fd2rpc, fd_str, (char *)r);
    hash_set(rpc->dict_uuid2fd, uuid_str, fdval);
    printf("add connection fd:%s, uuid:%s\n", fd_str, uuid_str);
    return 0;
}

int rpc_connect_del(struct rpc *rpc, int fd, uint32_t uuid)
{
    char uuid_str[9];
    char fd_str[9];
    snprintf(fd_str, sizeof(fd_str), "%08x", fd);
    snprintf(uuid_str, sizeof(uuid_str), "%08x", uuid);
    hash_del(rpc->dict_fd2rpc, fd_str);
    hash_del(rpc->dict_uuid2fd, uuid_str);
    return 0;
}

int create_uuid(char *uuid, int len, int fd, uint32_t ip, uint16_t port)
{
    snprintf(uuid, MAX_UUID_LEN, "%08x%08x%04x", fd, ip, port);
    return 0;
}

void rpc_connect_destroy(struct rpc *rpc, struct rpc *r)
{
    if (!rpc || !r) {
        printf("invalid paramets!\n");
        return;
    }
    int fd = r->fd;
    uint32_t uuid = r->send_pkt.header.uuid_src;
    struct gevent *e = r->ev;
    rpc_connect_del(rpc, fd, uuid);
    gevent_del(rpc->evbase, e);
}

void on_recv(int fd, void *arg)
{
    struct rpc *rpc = (struct rpc *)arg;
    struct iovec *buf;
    char key[9];
    snprintf(key, sizeof(key), "%08x", fd);
    struct rpc *r = (struct rpc *)hash_get(rpc->dict_fd2rpc, key);
    if (!r) {
        printf("hash_get failed: key=%s", key);
        return;
    }
    buf = rpc_recv_buf(r);
    if (!buf) {
        printf("on_disconnect fd = %d\n", r->fd);
        //rpc_connect_destroy(rpc, r);
        return;
    }
    do_process_msg(r, buf->iov_base, buf->iov_len);
    r->fd = fd;//must be reset
    free(buf->iov_base);
    free(buf);
}

void on_server_init(struct rpc *r, int fd, uint32_t ip, uint16_t port)
{
    char uuid[MAX_UUID_LEN];
    uint32_t uuid_hash;
    struct time_info ti;
    int ret;

    r->fd = fd;
    create_uuid(uuid, MAX_UUID_LEN, fd, ip, port);
    uuid_hash = hash_gen32(uuid, sizeof(uuid));
    struct gevent *e = gevent_create(fd, on_recv, NULL, NULL, (void *)r);
    if (-1 == gevent_add(r->evbase, e)) {
        printf("event_add failed!\n");
    }
    r->ev = e;

    r->send_pkt.header.uuid_src = uuid_hash;
    r->send_pkt.header.uuid_dst = uuid_hash;
    r->send_pkt.header.msg_id = 0;
    r->send_pkt.header.payload_len = sizeof(uuid_hash);
    r->send_pkt.payload = &uuid_hash;
    time_info(&ti);
    r->send_pkt.header.timestamp =  ti.utc_msec;
    printf("%s:%d before rpc_send\n", __func__, __LINE__);
    ret = rpc_send(r, r->send_pkt.payload, r->send_pkt.header.payload_len);
    if (ret == -1) {
        printf("rpc_send failed\n");
    }
    rpc_connect_add(r, r, fd, uuid_hash);
}

struct rpc *rpc_server_create(const char *host, uint16_t port)
{
    struct rpc *rpc = CALLOC(1, struct rpc);
    if (!rpc) {
        printf("malloc rpc failed!\n");
        return NULL;
    }

    rpc->ops = rpc_backend_list[RPC_BACKEND].ops;
    rpc->dict_fd2rpc = hash_create(10240);
    rpc->dict_uuid2fd = hash_create(10240);
    rpc->wq = wq_create();
    rpc->ops->register_recv_cb(rpc, process_msg);
    rpc->on_server_init = on_server_init;
    rpc->ctx = rpc->ops->init(rpc, host, port, RPC_SERVER);
    if (!rpc->ctx) {
        printf("init failed!\n");
        goto failed;
    }
    printf("rpc_server_create success\n");
    return rpc;

failed:
    if (rpc) {
        free(rpc);
    }
    return NULL;
}

void rpc_destroy(struct rpc *r)
{
    if (!r) {
        return;
    }
    sem_destroy(&r->sem);
    gevent_base_loop_break(r->evbase);
    gevent_base_destroy(r->evbase);
    r->ops->deinit(r);
    wq_destroy(r->wq);
    thread_destroy(r->dispatch_thread);
    free(r);
}
