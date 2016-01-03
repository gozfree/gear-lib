/*****************************************************************************
 * Copyright (C) 2014-2015
 * file:    librpc.c
 * author:  gozfree <gozfree@163.com>
 * created: 2015-05-07 00:00
 * updated: 2015-08-01 23:51
 *****************************************************************************/
#include <string.h>
#include <sys/time.h>
#include <libgzf.h>
#include <liblog.h>
#include <libgevent.h>
#include <libskt.h>
#include <libthread.h>
#include "librpc.h"

#define MAX_MESSAGES_IN_MAP         (256)
#define MAX_RPC_PACKET_SIZE         (1024)
#define MAX_UUID_LEN                (21)
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

int rpc_send(struct rpc *r, const void *buf, size_t len)
{
    int ret, head_size;
    struct timeval cur;
    struct rpc_packet *pkt = &r->packet;

    gettimeofday(&cur, NULL);
    //update time_stamp and payload_len
    pkt->header.payload_len = len;
    pkt->header.time_stamp = cur.tv_sec * 1000000L + cur.tv_usec;
    pkt->payload = (void *)buf;
    head_size = sizeof(rpc_header_t);
    ret = skt_send(r->fd, (void *)&pkt->header, head_size);
    if (ret != head_size) {
        loge("skt_send failed!\n");
        return -1;
    }
    ret = skt_send(r->fd, pkt->payload, pkt->header.payload_len);
    if (ret != pkt->header.payload_len) {
        loge("skt_send failed!\n");
        return -1;
    }
    //logi("xxx\n");
    //dump_packet(pkt);
    return (head_size + ret);
}

struct iovec *rpc_recv_buf(struct rpc *r)
{
    struct iovec *buf = CALLOC(1, struct iobuf);
    struct rpc_packet *pkt = &r->resp_pkt;
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
    if (r->packet.header.uuid_dst != pkt->header.uuid_dst) {
        loge("uuid_dst is diff from recved packet.header.uuid_dst!\n");
    }
    //strncpy(r->uuid_dst, pkt->header.uuid_dst, MAX_UUID_LEN);
    rlen = pkt->header.payload_len;
    buf->iov_base = calloc(1, rlen);
    buf->iov_len = rlen;
    pkt->payload = buf->iov_base;
    ret = skt_recv(r->fd, buf->iov_base, rlen);
    if (ret == 0) {
        loge("peer connect closed\n");
        goto err;
    } else if (ret != rlen) {
        loge("skt_recv failed: rlen=%d, ret=%d\n", rlen, ret);
        goto err;
    }
    loge("xxx\n");
    dump_buffer(buf->iov_base, ret);
    //logi("xxx\n");
    //dump_packet(pkt);
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
    struct rpc_packet *pkt = &r->resp_pkt;
    int head_size = sizeof(rpc_header_t);
    memset(pkt, 0, sizeof(rpc_packet_t));
    pkt->payload = buf;

    //step.1 recv response packet
    ret = skt_recv(r->fd, (void *)&pkt->header, head_size);
    if (ret == 0) {
        loge("peer connect closed\n");
        return -1;
    } else if (ret != head_size) {
        loge("skt_recv failed, head_size = %d, ret = %d\n", head_size, ret);
        return -1;
    }
    if (r->packet.header.uuid_dst != pkt->header.uuid_dst) {
        loge("uuid_dst is diff from recved packet.header.uuid_dst!\n");
    }
    if (len < pkt->header.payload_len) {
        loge("skt_recv pkt.header.len = %d\n", pkt->header.payload_len);
    }
    rlen = min(len, pkt->header.payload_len);
    ret = skt_recv(r->fd, buf, rlen);
    if (ret == 0) {
        loge("peer connect closed\n");
        return -1;
    } else if (ret != rlen) {
        loge("skt_recv failed: rlen=%d, ret=%d\n", rlen, ret);
        return -1;
    }
    //logi("xxx\n");
    //dump_packet(pkt);
    return ret;
}

static int unpack_msg(struct rpc_packet *pkt, uint32_t *msg_id,
                void *out_arg, size_t *out_len)
{
    struct rpc_header *hdr;
    if (!pkt || !out_arg || !out_len) {
        loge("invalid paraments!\n");
        return -1;
    }
    hdr = &(pkt->header);
    *msg_id = hdr->msg_id;
    *out_len = hdr->payload_len;
    memcpy(out_arg, pkt->payload, *out_len);
    return 0;
}

int process_msg(struct rpc *r, struct iovec *buf)
{
    msg_handler_t msg_handler;

    int msg_id = rpc_packet_parse(r);
    logi("msg_id = 0x%08x\n", msg_id);
    if (find_msg_handler(msg_id, &msg_handler) == 0 ) {
        msg_handler.cb(r, buf->iov_base, buf->iov_len);
    } else {
        loge("no callback for this MSG ID(0x%08x) in process_msg\n", msg_id);
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
    memset(&r->resp_pkt, 0, sizeof(struct rpc_packet));

    //step.1 recv response packet
#if 0
    int len = rpc_recv(r, &(r->resp_pkt), sizeof(struct rpc_packet));
    if (len == -1) {
        loge("rpc_recv failed!\n");
        return;
    }
#endif
    recv_buf = rpc_recv_buf(r);
    if (!recv_buf) {
        loge("rpc_recv_buf failed!\n");
        return;
    }
    print_packet(&r->resp_pkt);

    struct rpc_packet *pkt = &r->resp_pkt;

    //step.2 unpack packet
    unpack_msg(pkt, &msg_id, &out_arg, &out_len);
    logi("msg_id = 0x%08x\n", msg_id);

    if (r->state == rpc_inited) {
        logi("r->state is rpc_inited\n");
        r->packet.header.uuid_src = *(uint32_t *)pkt->payload;
        logi("r->packet.header.uuid_src = 0x%08x\n", r->packet.header.uuid_src);
        sem_post(&r->sem);
        r->state = rpc_connected;
    } else if (r->state == rpc_connected) {
        logi("r->state is rpc_connected\n");
        struct iovec buf;
        buf.iov_len = pkt->header.payload_len;
        buf.iov_base = pkt->payload;
        process_msg(r, &buf);
        //free(pkt->payload);
        sem_post(&r->sem);
    }
}

static void on_write(int fd, void *arg)
{
    loge("on_write fd= %d\n", fd);
}

static void on_error(int fd, void *arg)
{
    loge("on_error fd= %d\n", fd);
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
    char str_ip[INET_ADDRSTRLEN];
    struct rpc *r = CALLOC(1, struct rpc);
    if (!r) {
        loge("malloc failed!\n");
        return NULL;
    }
    if (-1 == sem_init(&r->sem, 0, 0)) {
        loge("sem_init failed %d: %s\n", errno, strerror(errno));
        return NULL;
    }
    memset(&r->resp_pkt, 0, sizeof(struct rpc_packet));
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

    skt_addr_ntop(str_ip, connect->local.ip);
    logi("local addr = %s:%d, uuid_src = %d\n", str_ip, connect->local.port, r->packet.header.uuid_src);
    skt_addr_ntop(str_ip, connect->remote.ip);

    //rpc_recv_buf(r);
    //logd("remote ip = %s, port = %d\n", str_ip, sc->remote.port);
    r->evbase = gevent_base_create();
    if (!r->evbase) {
        loge("gevent_base_create failed!\n");
        return NULL;
    }
    rpc_set_cb(r, on_read, on_write, on_error, r);
    r->dispatch_thread = thread_create("rpc_dispatch", rpc_dispatch_thread, r);

    r->state = rpc_inited;
    struct timeval now;
    struct timespec abs_time;
    uint32_t timeout = 20000;//msec
    gettimeofday(&now, NULL);
    /* Add our timeout to current time */
    now.tv_usec += (timeout % 1000) * 1000;
    now.tv_sec += timeout / 1000;
    /* Wrap the second if needed */
    if ( now.tv_usec >= 1000000 ) {
        now.tv_usec -= 1000000;
        now.tv_sec ++;
    }
    /* Convert to timespec */
    abs_time.tv_sec = now.tv_sec;
    abs_time.tv_nsec = now.tv_usec * 1000;
    if (-1 == sem_timedwait(&r->sem, &abs_time)) {
        loge("response failed %d:%s\n", errno, strerror(errno));
        return NULL;
    }
    logi("rpc_create success\n");
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
#if 0
        if (!pkt->payload) {
            loge("pky->payload is null!\n");
        } else {
            memcpy(pkt->payload, in_arg, in_len);
        }
#endif
    } else {
        hdr->payload_len = 0;
    }
    pkt_len = sizeof(struct rpc_packet) + hdr->payload_len;
    return pkt_len;
}

int rpc_packet_parse(struct rpc *r)
{
    struct rpc_header *h = &r->resp_pkt.header;
    return h->msg_id;
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

int rpc_call(struct rpc *r, uint32_t msg_id,
                const void *in_arg, size_t in_len,
                void *out_arg, size_t out_len)
{
    if (!r) {
        loge("invalid parament!\n");
        return -1;
    }
    logi("msg_id = 0x%08x\n", msg_id);
    size_t pkt_len = pack_msg(&r->packet, msg_id, in_arg, in_len);
    if (pkt_len == 0) {
        loge("pack_msg failed!\n");
        return -1;
    }
    if (0 > rpc_send(r, in_arg, in_len)) {
        loge("skt_send failed, fd = %d!\n", r->fd);
        return -1;
    }
    if (IS_RPC_MSG_NEED_RETURN(msg_id)) {
        struct timeval now;
        struct timespec abs_time;
        uint32_t timeout = 2000;//msec
        gettimeofday(&now, NULL);
        /* Add our timeout to current time */
        now.tv_usec += (timeout % 1000) * 1000;
        now.tv_sec += timeout / 1000;
        /* Wrap the second if needed */
        if ( now.tv_usec >= 1000000 ) {
            now.tv_usec -= 1000000;
            now.tv_sec ++;
        }
        /* Convert to timespec */
        abs_time.tv_sec = now.tv_sec;
        abs_time.tv_nsec = now.tv_usec * 1000;
        if (-1 == sem_timedwait(&r->sem, &abs_time)) {
            loge("response failed %d:%s\n", errno, strerror(errno));
            return -1;
        }
        memcpy(out_arg, r->resp_pkt.payload, out_len);
    } else {

    }
    return 0;
}
