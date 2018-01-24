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
#include <string.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <libmacro.h>
#include <liblog.h>
#include <libgevent.h>
#include <libdict.h>
#include <libskt.h>
#include <libthread.h>
#include "librpc.h"

static dict *_msg_map_registered = NULL;

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
    logi("packet header:\n");
    dump_buffer(&r->header, (int)sizeof(struct rpc_header));
    logi("packet data:\n");
    dump_buffer(r->payload, r->header.payload_len);
}

void print_packet(struct rpc_packet *pkt)
{
    logi("rpc_packet[%p]:\n", pkt);
    logi("header.uuid_dst    = 0x%08x\n", pkt->header.uuid_dst);
    logi("header.uuid_src    = 0x%08x\n", pkt->header.uuid_src);
    logi("header.msg_id      = 0x%08x\n", pkt->header.msg_id);
    logi("header.time_stamp  = 0x%08x\n", pkt->header.time_stamp);
    logi("header.payload_len = %d\n", pkt->header.payload_len);
    logi("header.checksum    = 0x%08x\n", pkt->header.checksum);
    logi("rpc_packet.payload:\n");
    dump_buffer(pkt->payload, pkt->header.payload_len);
}

static size_t pack_msg(struct rpc_packet *pkt, uint32_t msg_id,
                const void *in_arg, size_t in_len)
{
    struct rpc_header *hdr;
    struct timeval curtime;
    size_t pkt_len;
    if (!pkt) {
        loge("invalid paraments!\n");
        return 0;
    }
    gettimeofday(&curtime, NULL);

    hdr = &(pkt->header);
    hdr->msg_id = msg_id;
    hdr->time_stamp = curtime.tv_sec * 1000000L + curtime.tv_usec;

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
        loge("invalid paraments!\n");
        return -1;
    }
    hdr = &(pkt->header);
    *msg_id = hdr->msg_id;
    *out_len = hdr->payload_len;
    memcpy(out_arg, pkt->payload, *out_len);
    pkt_len = sizeof(struct rpc_packet) + hdr->payload_len;
    return pkt_len;
}

static void update_rpc_timestamp(struct rpc_packet *pkt)
{
    struct timeval cur;
    gettimeofday(&cur, NULL);
    pkt->header.time_stamp = cur.tv_sec * 1000000L + cur.tv_usec;
}

int rpc_send(struct rpc *r, const void *buf, size_t len)
{
    int ret, head_size;
    struct rpc_packet *pkt = &r->send_pkt;

    update_rpc_timestamp(pkt);
    pkt->header.payload_len = len;
    pkt->payload = (void *)buf;
    head_size = sizeof(rpc_header_t);
    ret = skt_send(r->fd, (void *)&pkt->header, head_size);
    if (ret != head_size) {
        loge("skt_send failed!\n");
        return -1;
    }
    ret = skt_send(r->fd, pkt->payload, pkt->header.payload_len);
    if (ret != (int)pkt->header.payload_len) {
        loge("skt_send failed!\n");
        return -1;
    }
    return (head_size + ret);
}

struct iovec *rpc_recv_buf(struct rpc *r)
{
    struct iovec *buf = CALLOC(1, struct iovec);
    struct rpc_packet *recv_pkt = &r->recv_pkt;
    uint32_t uuid_dst;
    uint32_t uuid_src;
    int ret;
    int head_size = sizeof(rpc_header_t);

    ret = skt_recv(r->fd, (void *)&recv_pkt->header, head_size);
    if (ret == 0) {
        //loge("peer connect closed\n");
        goto err;
    } else if (ret != head_size) {
        loge("skt_recv failed, head_size = %d, ret = %d\n", head_size, ret);
        goto err;
    }
    uuid_src = r->recv_pkt.header.uuid_src;
    uuid_dst = r->send_pkt.header.uuid_src;
    if (uuid_src != 0 && uuid_dst != 0 && uuid_src != uuid_dst) {
        logw("uuid_src(0x%08x) is diff from received uuid_dst(0x%08x)\n", uuid_src, uuid_dst);
        logw("this maybe a peer call\n");
    } else {
        //loge("uuid match.\n");
    }
    buf->iov_len = recv_pkt->header.payload_len;
    buf->iov_base = calloc(1, buf->iov_len);
    recv_pkt->payload = buf->iov_base;
    ret = skt_recv(r->fd, buf->iov_base, buf->iov_len);
    if (ret == 0) {
        loge("peer connect closed\n");
        goto err;
    } else if (ret != (int)buf->iov_len) {
        loge("skt_recv failed: rlen=%d, ret=%d\n", buf->iov_len, ret);
        goto err;
    }
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

    ret = skt_recv(r->fd, (void *)&pkt->header, head_size);
    if (ret == 0) {
        loge("peer connect closed\n");
        return -1;
    } else if (ret != head_size) {
        loge("skt_recv failed, head_size = %d, ret = %d\n", head_size, ret);
        return -1;
    }
    if (r->send_pkt.header.uuid_dst != pkt->header.uuid_dst) {
        loge("uuid_dst is diff from recved packet.header.uuid_dst!\n");
    }
    if (len < pkt->header.payload_len) {
        loge("skt_recv pkt.header.len = %d\n", pkt->header.payload_len);
    }
    rlen = MIN2(len, pkt->header.payload_len);
    ret = skt_recv(r->fd, buf, rlen);
    if (ret == 0) {
        loge("peer connect closed\n");
        return -1;
    } else if (ret != rlen) {
        loge("skt_recv failed: rlen=%d, ret=%d\n", rlen, ret);
        return -1;
    }
    return ret;
}

int process_msg(struct rpc *r, struct iovec *buf)
{
    msg_handler_t *msg_handler;

    int msg_id = rpc_packet_parse(r);
    msg_handler = find_msg_handler(msg_id);
    if (msg_handler) {
        msg_handler->cb(r, buf->iov_base, buf->iov_len);
    } else {
        //loge("no callback for this MSG ID(0x%08x) in process_msg\n", msg_id);
    }
    return 0;
}

static void on_read(int fd, void *arg)
{
    struct rpc *r = (struct rpc *)arg;
    char out_arg[1024];
    struct iovec *recv_buf;
    uint32_t msg_id;
    size_t out_len;
    memset(&r->recv_pkt, 0, sizeof(struct rpc_packet));

    recv_buf = rpc_recv_buf(r);
    if (!recv_buf) {
        loge("rpc_recv_buf failed!\n");
        return;
    }

    struct rpc_packet *pkt = &r->recv_pkt;
    unpack_msg(pkt, &msg_id, &out_arg, &out_len);
    logi("msg_id = %08x\n", msg_id);

    if (r->state == rpc_inited) {
        r->send_pkt.header.uuid_src = *(uint32_t *)pkt->payload;
        thread_sem_signal(r->dispatch_thread);
        r->state = rpc_connected;
    } else if (r->state == rpc_connected) {
        struct iovec buf;
        buf.iov_len = pkt->header.payload_len;
        buf.iov_base = pkt->payload;
        process_msg(r, &buf);
        //free(pkt->payload);
        thread_sem_signal(r->dispatch_thread);
    }
}

static void on_write(int fd, void *arg)
{
//    loge("on_write fd= %d\n", fd);
}

static void on_error(int fd, void *arg)
{
//    loge("on_error fd= %d\n", fd);
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

struct rpc *rpc_create(const char *host, uint16_t port)
{
    char local_ip[INET_ADDRSTRLEN];
    char remote_ip[INET_ADDRSTRLEN];
    struct rpc *r = CALLOC(1, struct rpc);
    if (!r) {
        loge("malloc failed!\n");
        return NULL;
    }
    memset(&r->recv_pkt, 0, sizeof(struct rpc_packet));
    struct skt_connection *connect;
    connect = skt_tcp_connect(host, port);
    if (!connect) {
        loge("connect failed!\n");
        return NULL;
    }
    r->fd = connect->fd;
    if (-1 == skt_set_block(r->fd)) {
        loge("skt_set_block failed!\n");
    }

    r->evbase = gevent_base_create();
    if (!r->evbase) {
        loge("gevent_base_create failed!\n");
        return NULL;
    }
    rpc_set_cb(r, on_read, on_write, on_error, r);
    r->dispatch_thread = thread_create(rpc_dispatch_thread, r);

    r->state = rpc_inited;
    if (thread_sem_wait(r->dispatch_thread, 2000) == -1) {
        loge("wait response failed %d:%s\n", errno, strerror(errno));
        return NULL;
    }

    skt_addr_ntop(local_ip, connect->local.ip);
    skt_addr_ntop(remote_ip, connect->remote.ip);
    logi("rpc[%08x] connect information:\n", r->send_pkt.header.uuid_src);
    logi("local addr = %s:%d\n", local_ip, connect->local.port);
    logi("remote addr = %s:%d\n", remote_ip, connect->remote.port);

    return r;
}

void rpc_destroy(struct rpc *r)
{
    if (!r) {
        return;
    }
    gevent_base_destroy(r->evbase);
#if 0
    if (r->send_pkt.payload) {
        free(r->send_pkt.payload);
    }
    if (r->recv_pkt.payload) {
        free(r->recv_pkt.payload);
    }
#endif

    thread_destroy(r->dispatch_thread);
    free(r);
}

int rpc_set_cb(struct rpc *r,
        void (*cbr)(int fd, void *arg),
        void (*cbw)(int fd, void *arg),
        void (*cbe)(int fd, void *arg),  void *arg)
{
    if (!r) {
        loge("invalid parament!\n");
        return -1;
    }
    struct gevent *e = gevent_create(r->fd, cbr, cbw, cbe, arg);
    if (!e) {
        loge("gevent_create failed!\n");
    }
    if (-1 == gevent_add(r->evbase, e)) {
        loge("event_add failed!\n");
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
        loge("Cannot register null msg proc\n");
        return -1;
    }
    msg_id = handler->msg_id;
    snprintf(msg_id_str, sizeof(msg_id_str), "0x%08x", msg_id);
    msg_id_str[10] = '\0';
    if (!_msg_map_registered) {
            _msg_map_registered = dict_new();
            if (!_msg_map_registered) {
                    loge("create message map table failed!\n");
                    return -1;
            }
    }
    msg_proc = dict_get(_msg_map_registered, msg_id_str, NULL);
    if (msg_proc) {//force update
        if (0 != dict_del(_msg_map_registered, msg_id_str)) {
            loge("dict_del failed!\n");
        }
    }
    if (0 != dict_add(_msg_map_registered, msg_id_str, (char *)handler)) {
        loge("dict_add failed!\n");
    }
    return 0;
}

int register_msg_map(msg_handler_t *map, int num_entry)
{
    int i;
    int ret;

    if (map == NULL) {
        loge("register_msg_map: null map \n");
        return -1;
    }

    if (num_entry <= 0) {
        loge("register_msg_map:invalid num_entry %d \n", num_entry);
        return -1;
    }

    for (i = 0; i < num_entry; i++) {
        ret = register_msg_proc(&map[i]);
        if (ret < 0) {
            loge("register_msg_map:register failed  at %d \n", i);
            return -1;
        }
    }

    return 0;
}

msg_handler_t *find_msg_handler(uint32_t msg_id)
{
    char msg_id_str[11];
    msg_handler_t *handler;
    snprintf(msg_id_str, sizeof(msg_id_str), "0x%08x", msg_id);
    msg_id_str[10] = '\0';
    handler = (msg_handler_t *)dict_get(_msg_map_registered, msg_id_str, NULL);
    return handler;

}

int rpc_call(struct rpc *r, uint32_t msg_id,
                const void *in_arg, size_t in_len,
                void *out_arg, size_t out_len)
{
    if (!r) {
        loge("invalid parament!\n");
        return -1;
    }
    size_t pkt_len = pack_msg(&r->send_pkt, msg_id, in_arg, in_len);
    if (pkt_len == 0) {
        loge("pack_msg failed!\n");
        return -1;
    }
    if (0 > rpc_send(r, in_arg, in_len)) {
        loge("skt_send failed, fd = %d!\n", r->fd);
        return -1;
    }
    if (IS_RPC_MSG_NEED_RETURN(msg_id)) {
        if (thread_sem_wait(r->dispatch_thread, 2000) == -1) {
            loge("wait response failed %d:%s\n", errno, strerror(errno));
            return -1;
        }
        logi("recv_pkt.len = %d\n", r->recv_pkt.header.payload_len);
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


