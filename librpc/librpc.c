/*****************************************************************************
 * Copyright (C) 2014-2015
 * file:    librpc.c
 * author:  gozfree <gozfree@163.com>
 * created: 2015-05-07 00:00
 * updated: 2015-08-01 23:51
 *****************************************************************************/
#include <string.h>
#include <sys/time.h>
#include <libskt.h>
#include <libgzf.h>
#include <libgevent.h>
#include <libglog.h>
#include "librpc.h"

static msg_handler_t message_map[MAX_MESSAGES_IN_MAP];
static int           message_map_registered;



void dump_buffer(void *buf, int len)
{
    int i;
    for (i = 0; i < len; i++) {
        if (!(i%16))
           printf("\n%p: ", buf+i);
        printf("%02x ", (*((char *)buf + i)) & 0xff);
    }
    printf("\n");
}

void dump_packet(struct rpc_packet *r)
{
    printf("packet header:");
    dump_buffer(&r->header, (int)sizeof(struct rpc_header));
    printf("packet data:");
    dump_buffer(r->data, r->header.len);
}

int rpc_send(struct rpc *r, const void *buf, size_t len)
{
    int ret, head_size;
    struct rpc_packet *pkt = &r->packet;
    pkt->header.len = len;
    pkt->data = (void *)buf;
    head_size = sizeof(rpc_header_t);
    ret = skt_send(r->fd, (void *)&pkt->header, head_size);
    if (ret != head_size) {
        loge("skt_send failed!\n");
        return -1;
    }
    ret = skt_send(r->fd, buf, len);
    if (ret != len) {
        loge("skt_send failed!\n");
        return -1;
    }
    return ret;
}
struct iobuf *rpc_recv_buf(struct rpc *r)
{
    struct iobuf *buf = CALLOC(1, struct iobuf);
    struct rpc_packet *pkt = &r->packet;
    int ret, rlen;
    int head_size = sizeof(rpc_header_t);

    ret = skt_recv(r->fd, (void *)&pkt->header, head_size);
    if (ret == 0) {
        loge("peer connect closed\n");
        goto err;
    } else if (ret != head_size) {
        loge("skt_recv failed, head_size = %d, ret = %d\n", head_size, ret);
        goto err;
    }
    //strncpy(r->uuid_dst, pkt->header.uuid_dst, MAX_UUID_LEN);
    rlen = pkt->header.len;
    buf->addr = calloc(1, rlen);
    buf->len = rlen;
    pkt->data = buf->addr;
    ret = skt_recv(r->fd, buf->addr, rlen);
    if (ret == 0) {
        loge("peer connect closed\n");
        goto err;
    } else if (ret != rlen) {
        loge("skt_recv failed: rlen=%d, ret=%d\n", rlen, ret);
        goto err;
    }
    //dump_packet(&pkt);
    return buf;
err:
    if (buf->addr) {
        free(buf->addr);
    }
    free(buf);
    return NULL;
}

int rpc_recv(struct rpc *r, void *buf, size_t len)
{
    struct rpc_packet pkt;
    int ret, rlen;
    int head_size = sizeof(rpc_header_t);
    memset(&pkt, 0, sizeof(rpc_packet_t));
    pkt.data = buf;

    ret = skt_recv(r->fd, (void *)&pkt.header, head_size);
    if (ret == 0) {
        loge("peer connect closed\n");
        return -1;
    } else if (ret != head_size) {
        loge("skt_recv failed, head_size = %d, ret = %d\n", head_size, ret);
        return -1;
    }
    //strncpy(r->uuid_dst, pkt.header.uuid_dst, MAX_UUID_LEN);
    if (len < pkt.header.len) {
        loge("skt_recv pkt.header.len = %d\n", pkt.header.len);
    }
    rlen = min(len, pkt.header.len);
    ret = skt_recv(r->fd, buf, rlen);
    if (ret == 0) {
        loge("peer connect closed\n");
        return -1;
    } else if (ret != rlen) {
        loge("skt_recv failed: rlen=%d, ret=%d\n", rlen, ret);
        return -1;
    }
    //dump_packet(&pkt);
    return ret;
}

struct rpc *rpc_create(const char *host, uint16_t port)
{
    int ret;
    char str_ip[INET_ADDRSTRLEN];
    struct skt_connection *sc;
    struct rpc *r = CALLOC(1, struct rpc);
    if (!r) {
        loge("malloc failed!\n");
        return NULL;
    }
    sc = skt_tcp_connect(host, port);
    if (sc == NULL) {
        loge("connect failed!\n");
        return NULL;
    }
    r->fd = sc->fd;

#if 1
    struct skt_addr tmpaddr;
    char tmpip[INET_ADDRSTRLEN];
    if (-1 == skt_getaddr_by_fd(sc->fd, &tmpaddr)) {
    }
    skt_addr_ntop(tmpip, tmpaddr.ip);
    loge("addr = %s:%d\n", tmpip, tmpaddr.port);

#endif
    if (-1 == skt_set_noblk(sc->fd, 0)) {
        loge("block skt_recv failed!\n");
    }
    ret = rpc_recv(r, r->packet.header.uuid_src, MAX_UUID_LEN);
    if (ret != MAX_UUID_LEN) {
        loge("rpc_recv failed: ret = %d\n", ret);
    }
//    if (-1 == skt_set_noblk(sc->fd, 1)) {
//        printf("no-block skt_recv failed!\n");
//    }
    skt_addr_ntop(str_ip, sc->local.ip);
    loge("local addr = %s:%d, uuid_src = %s\n", str_ip, sc->local.port, r->packet.header.uuid_src);
    skt_addr_ntop(str_ip, sc->remote.ip);
    //printf("remote ip = %s, port = %d\n", str_ip, sc->remote.port);
    r->evbase = gevent_base_create();

    return r;
}

void rpc_destroy(struct rpc *r)
{

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

int rpc_echo(struct rpc *r, const void *buf, size_t len)
{
    return skt_send(r->fd, buf, len);
}

int rpc_dispatch(struct rpc *r)
{
    gevent_base_loop(r->evbase);
    return 0;
}

//===============

int rpc_header_format(struct rpc *r,
        char *uuid_dst, char *uuid_src, uint32_t type, size_t len)
{
    if (!r) {
        loge("invalid parament\n");
        return -1;
    }
    struct timeval cur;
    struct rpc_header *h = &r->packet.header;
    if (!h) {
        loge("rpc_header is NULL\n");
        return -1;
    }
    if (uuid_src) {
        snprintf(h->uuid_src, MAX_UUID_LEN, "%s", uuid_src);
    }
    if (uuid_dst) {
        snprintf(h->uuid_dst, MAX_UUID_LEN, "%s", uuid_dst);
    }
    gettimeofday(&cur, NULL);
    h->time_stamp = cur.tv_sec * 1000000L + cur.tv_usec;
    h->type = type;
    h->len = len;
    return 0;
}

int rpc_packet_format(struct rpc *r, uint32_t type)
{
    struct rpc_header *h = &r->packet.header;
    //memset(h, 0, sizeof(rpc_packet_t));
    //strncpy(h->uuid_dst, r->uuid_dst, MAX_UUID_LEN);
    //strncpy(h->uuid_src, r->uuid_src, MAX_UUID_LEN);
    logi("uuid_src=%s, uuid_dst=%s\n", h->uuid_src, h->uuid_dst);
    h->type = type;
    return 0;
}

int rpc_packet_parse(struct rpc *r)
{
    struct rpc_header *h = &r->packet.header;
    return h->type;
}

int register_msg_proc(msg_handler_t *handler)
{
    int i;
    int msg_id_registered  = 0;
    int msg_slot = -1;
    uint32_t msg_id;

    if (!handler) {
        loge("Cannot register null msg proc \n");
        return -1;
    }

    msg_id = handler->msg_id;

    for (i=0; i < message_map_registered; i++) {
        if (message_map[i].msg_id == msg_id) {
            loge("overwrite existing msg proc for msg_id %d \n", msg_id);
            msg_id_registered = 1;
            msg_slot = i;
            break;
        }
    }

    if ((!msg_id_registered)) {
        if ((message_map_registered == MAX_MESSAGES_IN_MAP)) {
            loge("too many msg proc registered \n");
            return -1;
        }
        //increase the count
        msg_slot = message_map_registered;
        message_map_registered++;
    }

    //if the handler registered is NULL, then just fill NULL handler
    message_map[msg_slot] = *handler;

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

    if ((num_entry <= 0) || (num_entry > MAX_MESSAGES_IN_MAP)) {
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

int find_msg_handler(uint32_t msg_id, msg_handler_t *handler)
{
    int i;
    for (i=0; i< message_map_registered; i++) {
        if (message_map[i].msg_id == msg_id) {
            if (handler)  {
                *handler = message_map[i];
            }
            return 0;
        }
    }
    return -1;
}

int process_msg2(struct rpc *r, struct iobuf *buf)
{
    msg_handler_t msg_handler;

    int msg_id = rpc_packet_parse(r);
//    logi("msg_id = %x\n", msg_id);
    if (find_msg_handler(msg_id, &msg_handler) == 0 ) {
        msg_handler.cb(r, buf->addr, buf->len);
    } else {
        loge("no callback for this MSG ID in process_msg\n");
    }
    return 0;
}



int rpc_call(struct rpc *r, int func_id,
            void *in_arg, size_t in_len,
            void *out_arg, size_t out_len)
{
    rpc_packet_format(r, func_id);
    return rpc_send(r, in_arg, in_len);
}
