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
#include <libgevent.h>
#include <libhash.h>
#include <libthread.h>
#include <libworkq.h>
#include <libtime.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <sys/time.h>
#include <sys/uio.h>

extern struct rpc_ops socket_ops;

struct rpc_backend {
    int id;
    struct rpc_ops *ops;
};

typedef enum rpc_backend_type {
    RPC_SOCKET,
    RPC_UART,
} rpc_backend_type;

static struct rpc_backend rpc_backend_list[] = {
    {RPC_SOCKET,   &socket_ops},
};

#define RPC_BACKEND RPC_SOCKET
#define MAX_UUID_LEN                (21)

struct wq_arg {
    msg_handler_t handler;
    struct rpc_session session;
    void *buf;
    size_t len;
};

static struct hash *_msg_map_registered = NULL;

static void dump_buffer(void *buf, int len)
{
    int i;
    for (i = 0; i < len; i++) {
        if (!(i%16))
           printf("\n%p: ", (char *)buf+i);
        printf("%02x ", (*((char *)buf + i)) & 0xff);
    }
    printf("\n");
}

static void dump_packet(struct rpc_packet *r)
{
    printf("packet header:");
    dump_buffer(&r->header, (int)sizeof(struct rpc_header));
    printf("packet payload:");
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
    printf("header.timestamp   = %" PRIu64 "(%s)\n", pkt->header.timestamp,
            time_str_human_by_msec(pkt->header.timestamp, ts, sizeof(ts)));
    printf("header.payload_len = %d\n", pkt->header.payload_len);
    printf("header.checksum    = 0x%08x\n", pkt->header.checksum);
    dump_packet(pkt);
    printf("================\n");
}

static void *event_thread(struct thread *t, void *arg)
{
    struct rpc_base *r = (struct rpc_base *)arg;
    gevent_base_loop(r->evbase);
    return NULL;
}

static int rpc_base_init(struct rpc_base *r)
{
    r->ops = rpc_backend_list[RPC_BACKEND].ops;
    r->evbase = gevent_base_create();
    if (!r->evbase) {
        printf("gevent_base_create failed!\n");
        return -1;
    }
    da_init(r->ev_list);
    r->dispatch_thread = thread_create(event_thread, r);
    if (!r->dispatch_thread) {
        printf("thread_create failed!\n");
        return -1;
    }
    return 0;
}

static void rpc_base_deinit(struct rpc_base *r)
{
    r->ops = NULL;
    gevent_base_loop_break(r->evbase);
    gevent_base_destroy(r->evbase);
    thread_destroy(r->dispatch_thread);
}

static size_t pack_msg(struct rpc_packet *pkt, uint32_t msg_id,
                const void *in_arg, size_t in_len)
{
    struct rpc_header *hdr;
    size_t pkt_len;
    struct time_info ti;

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

#if 0
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
#endif

static int rpc_send(struct rpc_base *r, struct rpc_packet *pkt)
{
    int ret, head_size;

    head_size = sizeof(rpc_header_t);

    printf("rpc_send >>>>\n");
    print_packet(pkt);
    ret = r->ops->send(r, (void *)&pkt->header, head_size);
    if (ret != head_size) {
        printf("send failed!\n");
        return -1;
    }
    ret = r->ops->send(r, pkt->payload, pkt->header.payload_len);
    if (ret != (int)pkt->header.payload_len) {
        printf("send failed!\n");
        return -1;
    }
    return (head_size + pkt->header.payload_len);
}

int rpc_recv(struct rpc_base *r, struct rpc_packet *pkt)
{
    int ret;
    int head_size = sizeof(rpc_header_t);

    ret = r->ops->recv(r, (void *)&pkt->header, head_size);
    if (ret == 0) {
        printf("peer connect closed\n");
        return -1;
    } else if (ret != head_size) {
        printf("recv failed, head_size = %d, ret = %d\n", head_size, ret);
        return -1;
    }
    pkt->payload = calloc(1, pkt->header.payload_len);
    printf("payload len=%d\n", pkt->header.payload_len);

    ret = r->ops->recv(r, pkt->payload, pkt->header.payload_len);
    if (ret == 0) {
        printf("peer connect closed\n");
        return -1;
    }
    printf("rpc_recv <<<<\n");
    print_packet(pkt);
    return ret;
}

static msg_handler_t *find_msg_handler(uint32_t msg_id)
{
    char msg_id_str[11];
    msg_handler_t *handler;
    snprintf(msg_id_str, sizeof(msg_id_str), "0x%08x", msg_id);
    msg_id_str[10] = '\0';
    handler = (msg_handler_t *)hash_get(_msg_map_registered, msg_id_str);
    return handler;

}

#if 0
static int async_msg_push(struct rpc *rpc, uint32_t func_id)
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
#endif

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



/******************************************************************************
 * client API
 ******************************************************************************/

#if 0
static int process_msg(struct rpc *r, void *buf, size_t len)
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



int rpc_peer_call(struct rpc *r, uint32_t uuid, uint32_t cmd_id,
            const void *in_arg, size_t in_len,
            void *out_arg, size_t out_len)
{
    r->send_pkt.header.uuid_dst = uuid;
    return rpc_call(r, cmd_id, in_arg, in_len, out_arg, out_len);
}
#endif

int rpc_call(struct rpc *r, uint32_t msg_id,
                const void *in_arg, size_t in_len,
                void *out_arg, size_t out_len)
{
    char cmd_str[11];//0xFFFFFFFF
    struct rpc_packet send_pkt;//, recv_pkt;
    if (!r) {
        printf("invalid parament!\n");
        return -1;
    }
    size_t pkt_len = pack_msg(&send_pkt, msg_id, in_arg, in_len);
    if (pkt_len == 0) {
        printf("pack_msg failed!\n");
        return -1;
    }
    if (-1 == rpc_send(&r->base, &send_pkt)) {
        printf("rpc_send failed\n");
        return -1;
    }
    if (IS_RPC_MSG_NEED_RETURN(msg_id)) {
        printf("wait message return!\n");
        snprintf(cmd_str, sizeof(cmd_str), "0x%08X", msg_id);
        if (-1 == hash_set(r->hash_async_cmd, cmd_str, cmd_str)) {
            return -1;
        }
        thread_lock(r->base.dispatch_thread);
        if (thread_wait(r->base.dispatch_thread, 2000) == -1) {
            printf("wait response failed %d:%s\n", errno, strerror(errno));
            return -1;
        }
        thread_unlock(r->base.dispatch_thread);
        //memcpy(out_arg, r->recv_pkt.payload, out_len);
    }
    return 0;
}

static void on_connect_to_server(struct rpc *rpc)
{
    char payload[1024];
    struct rpc_packet pkt;
    memset(&pkt, 0, sizeof(pkt));
    memset(payload, 0, sizeof(payload));
    pkt.payload = payload;
    pkt.header.payload_len = sizeof(payload);
    if (-1 == rpc_recv(&rpc->base, &pkt)) {
        printf("rpc_recv failed!\n");
        return;
    }
    if (rpc->state == rpc_inited) {
        rpc->uuid = *(uint32_t *)pkt.payload;
        thread_lock(rpc->base.dispatch_thread);
        thread_signal(rpc->base.dispatch_thread);
        thread_unlock(rpc->base.dispatch_thread);
        rpc->state = rpc_connected;
        printf("rpc connected uuid=0x%08x\n", rpc->uuid);
    } else if (rpc->state == rpc_connected) {
        printf("rpc already connected!\n");
#if 0
        struct iovec buf;
        buf.iov_len = pkt->header.payload_len;
        buf.iov_base = pkt->payload;
        process_msg(r, buf.iov_base, buf.iov_len);
        //free(pkt->payload);
#endif
    }
}

struct rpc *rpc_client_create(const char *host, uint16_t port)
{
    struct rpc *r = calloc(1, sizeof(struct rpc));
    if (!r) {
        printf("malloc failed!\n");
        return NULL;
    }
    if (rpc_base_init(&r->base) < 0) {
        printf("rpc_base_init failed!\n");
        return NULL;
    }

    r->hash_async_cmd = hash_create(1024);
#if 0
    r->resp_buf = calloc(1, MAX_RPC_MESSAGE_SIZE);
#endif
    r->on_connect_server = on_connect_to_server;
    if (r->base.ops->init_client(&r->base, host, port) < 0) {
        printf("init_client failed!\n");
    }
    r->state = rpc_inited;
    printf("rpc_client_create success\n");
    return r;
}

void rpc_client_destroy(struct rpc *r)
{
#if 0
    if (!r) {
        return;
    }
    sem_destroy(&r->sem);
    gevent_base_loop_break(r->evbase);
    gevent_base_destroy(r->evbase);
    r->ops->deinit(&r->base);
    wq_destroy(r->wq);
    thread_destroy(r->dispatch_thread);
    free(r);
#endif
}


/******************************************************************************
 * server API
 ******************************************************************************/
int rpc_session_del(struct rpcs *s, uint32_t uuid)
{
    struct rpc_session *session;
    session = hash_get32(s->hash_session, uuid);
    if (!session) {
        printf("rpc session %d does not exist!\n", uuid);
        return -1;
    }
    free(session);
    return 0;
}

void rpc_connect_destroy(struct rpcs *s, struct rpc *r)
{
#if 0
    if (!s || !r) {
        printf("invalid paramets!\n");
        return;
    }
    int fd = r->fd;
    uint32_t uuid = r->send_pkt.header.uuid_src;
    struct gevent *e = r->ev;
    rpc_connect_del(s, fd, uuid);
    gevent_del(s->evbase, e);
#endif
}

static void process_wq(void *arg)
{
    struct wq_arg *wq = (struct wq_arg *)arg;
    if (&wq->handler) {
        wq->handler.cb(&wq->session, wq->buf, wq->len);
    }
}

static int process_msg(struct rpcs *s, struct rpc_session *session, struct rpc_packet *pkt)
{
    int ret = 0;
    msg_handler_t *msg_handler;
    struct rpc_header *h = &pkt->header;

    printf("msg_id = %08x\n", h->msg_id);
    msg_handler = find_msg_handler(h->msg_id);
    if (msg_handler) {
        struct wq_arg *arg = calloc(1, sizeof(struct wq_arg));
        memcpy(&arg->handler, msg_handler, sizeof(msg_handler_t));
        memcpy(&arg->session, session, sizeof(struct rpc_session));
        arg->buf = memdup(pkt->payload, h->payload_len);
        arg->len = h->payload_len;
        wq_task_add(s->wq, process_wq, arg, sizeof(struct wq_arg));
    } else {
        printf("no callback for this MSG ID(%d) in process_msg\n", h->msg_id);
#if 0
        snprintf(uuid_str, sizeof(uuid_str), "%x", h->uuid_dst);
        char *valfd = (char *)hash_get(s->hash_uuid2fd, uuid_str);
        if (!valfd) {
            printf("hash_get failed: key=%x\n", h->uuid_dst);
            return -1;
        }
        int dst_fd = strtol(valfd, NULL, 16);
        r->fd = dst_fd;
        printf("%s:%d before rpc_send\n", __func__, __LINE__);
        ret = rpc_send(r, buf, len);
#endif
    }
    return ret;
}

static void on_message_from_client(struct rpcs *s, struct rpc_session *session)
{
    struct rpc_packet pkt;

    printf("before rpc_recv fd=%d, rpc_base=%p\n", session->base.fd, &session->base);
    if (rpc_recv(&session->base, &pkt) == -1) {
        printf("rpc_recv failed\n");
        return;
    }
    printf("after rpc_recv\n");
    process_msg(s, session, &pkt);
}

static struct rpc_session *on_create_session(struct rpcs *s, int fd, uint32_t uuid)
{
    struct rpc_session *session;
    struct time_info ti;
    struct rpc_packet pkt;
    int ret;

    session = hash_get32(s->hash_session, uuid);
    if (session) {
        printf("rpc session %d already exist!\n", uuid);
        return session;
    }
    session = (struct rpc_session *)calloc(1, sizeof(struct rpc_session));
    if (!session) {
        printf("malloc rpc_session failed!\n");
        return NULL;
    }
    memcpy(&session->base, &s->base, sizeof(struct rpc_base));
    session->base.fd = fd;
    session->uuid = uuid;
    session->cseq = 0;
    hash_set32(s->hash_session, uuid, session);

    memset(&pkt, 0, sizeof(pkt));
    pkt.header.uuid_src = uuid;
    pkt.header.uuid_dst = uuid;
    pkt.header.msg_id = 0;
    pkt.header.payload_len = sizeof(uuid);
    pkt.payload = &uuid;
    time_info(&ti);
    pkt.header.timestamp = ti.utc_msec;

    ret = rpc_send(&session->base, &pkt);
    if (ret == -1) {
        printf("%s: rpc_send failed\n", __func__);
    }
    return session;
}

struct rpcs *rpc_server_create(const char *host, uint16_t port)
{
    struct rpcs *s = calloc(1, sizeof(struct rpcs));
    if (!s) {
        printf("malloc rpcs failed!\n");
        return NULL;
    }
    if (rpc_base_init(&s->base) < 0) {
        printf("rpc_base_init failed!\n");
        goto failed;
    }

    s->hash_session = hash_create(1024);
    s->hash_fd2session = hash_create(10240);
    s->hash_uuid2fd = hash_create(10240);
    s->wq = wq_create();
    s->on_create_session = on_create_session;
    s->on_message = on_message_from_client;
    if (s->base.ops->init_server(&s->base, host, port) < 0) {
        printf("init_server failed!\n");
        goto failed;
    }
    printf("rpc_server_create success\n");
    return s;

failed:
    if (s) {
        free(s);
    }
    return NULL;
}

void rpc_server_destroy(struct rpcs *s)
{
    if (!s) {
        return;
    }
    s->base.ops->deinit(&s->base);
    wq_destroy(s->wq);
    rpc_base_deinit(&s->base);
    free(s);
}
